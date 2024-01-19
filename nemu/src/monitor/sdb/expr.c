/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>


enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUMBER,TK_NEGATIVE,TK_NOEQ,TK_AND,TK_POINTER_DEREF,TK_REG,TK_HEX,
 
  /* TODO: Add more token types */
 
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
 
  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
 
  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"!=",TK_NOEQ},       // NO equal
  {"&&",TK_AND},        // and
  {"-",'-'},            // minus
  {"\\*",'*'},          // multiplication
  {"/",'/'},            // divisions
  {"\\b[0-9]+\\b", TK_NUMBER},   //number 
  {"\\(", '('},         // 左括号
  {"\\)", ')'},         // 右括号
  //{"[A-Za-z]+",TK_STRING},//字符串
  {"\\$(\\$0|ra|[sgt]p|t[0-6]|a[0-7]|s([0-9]|1[0-1]))", TK_REG},//寄存器
  {"0[xX][0-9a-fA-F]+",TK_HEX},    //十六进制
  
};


#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
 
 //可以准确地记录正则表达式匹配的子字符串在原始字符串中的位置信息。
  regmatch_t pmatch;
 
  nr_token = 0;
 
  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;
 
        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
 
        position += substr_len;
 
        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
 
        Token token;
 
        switch (rules[i].token_type) 
        {
          case TK_NOTYPE:
          break;
          default: 
           strncpy(token.str, substr_start, substr_len);
           token.str[substr_len] = '\0'; 
           token.type=rules[i].token_type;
           tokens[nr_token++] = token;
           break;
        }
 
        break;
      }
    }
    
    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
 
  return true;
}


bool check_parentheses(word_t p,word_t q)
{
  bool flag=false;
  if(tokens[p].type=='(' && tokens[q].type == ')')
  {
    for(int i =p+1;i<q;)
    {
      
      if (tokens[i].type==')')
      {
        
        break;
      }
 
      else if (tokens[i].type=='(')
      {
        while(tokens[i+1].type!=')' )
        {
          i++;
          if(i==q-1)
          {
            
            break;
          }
        }
        i+=2;
      }
 
      else i++;
        
    }
    flag=true;
  }
  return flag;
}


word_t find_major(word_t p,word_t q)
{
  word_t ret=0;
  word_t par=0;//括号的数量
  word_t op_type=0; //当前找到的最高优先级的运算符类型
  word_t tmp_type=0; //相应运算符类型的等级
  for(word_t i=p;i<=q;i++)
  {
    if (tokens[i].type=='-')//负号处理
    {
      if(i==p)
      {
        tokens[i].type=TK_NEGATIVE;
        return i;
      }
      
    }
 
      //指针解引用处理
    for (int i = 0; i < nr_token; i ++) 
    {
    if (tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type !=')' || tokens[i - 1].type !=TK_NUMBER)) ) 
    {
      tokens[i].type = TK_POINTER_DEREF;
      return i;
    }
    }
    
    //数字
    if (tokens[i].type ==TK_NUMBER)
    {
      continue;
    }
 
    else if (tokens[i].type=='(')
    {
      par++;
      continue;
    }
 
    else if (tokens[i].type==')')
    {
      if (par==0)
      {
        return -1;
      }
      par--;
    }
    else if (par>0)
    {
      continue;
    }
    else
    {
      switch (tokens[i].type) 
      {
      case '*': case '/': tmp_type = 1; break;
      case '+': case '-': tmp_type = 2; break;
      case TK_EQ:case TK_NOEQ:tmp_type=3;break;
      case TK_AND:tmp_type=4;break;
      //cas
      default: assert(0);
      }
      if (tmp_type>=op_type)
      {
        op_type=tmp_type;
        ret=i;
      }
      
    }
 
  }
    if(par>0)
    {return -1;}
    return ret;
}
 
 
/**********************************************eval求值函数*******************************************************************/
int32_t eval(word_t p,word_t q)
{
    
    if(p>q)
    {
      printf("The input is wrong p>q\n");
      assert(0);
    }
    else if (p==q)
    {
      if (tokens[p].type == TK_REG) 
      {
      word_t num;
        bool t = true;
        num = isa_reg_str2val(tokens[p].str, &t);
        if (!t) 
        {
          num = 0;
        }
        return num;
      }
      else if (tokens[p].type==TK_NUMBER)
      {
        word_t num;
        sscanf(tokens[p].str,"%d",&num);
        return num;
      }
      else if (tokens[p].type==TK_HEX)
      {
        return strtol(tokens[p].str, NULL, 16);
      }
      else
      {
        printf("false when p==q");
        return 0;
      }
      
      
    }
    else if (check_parentheses(p, q) == true) 
    {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
    }
    else
    {
      word_t op=find_major(p,q);  //主运算符的索引
      
      int32_t val2 = eval(op + 1, q);  //可能是负数，所以先计算val2
      //负数处理
      if(tokens[op].type==TK_NEGATIVE)
      {
        val2=-val2;
        return val2;
      }
 
      //指针解引用       
      if(tokens[op].type==TK_POINTER_DEREF)
      {
        word_t* ptr = (word_t*)tokens[op+1].str;
        return *ptr;
      }
      
      int32_t val1 = eval(p, op - 1);//写在后是为了防止op是负数导致eval传入的p>q
 
       switch (tokens[op].type) 
      {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      case TK_EQ: return val1==val2;
      case TK_NOEQ:return val1!=val2;
      case TK_AND:return val1&&val2;
      default: assert(0);
      }
    }  
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  
  return eval(0, nr_token-1);
}

