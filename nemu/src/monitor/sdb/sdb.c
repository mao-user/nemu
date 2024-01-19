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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_END;
  return -1;
}

static int cmd_si(char *args) {
  int step = atoi(args);
  cpu_exec(step);
  return 0;
}

static int cmd_x(char *args) {
  char *arg1 = strtok(NULL, " ");
  if (arg1 == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }
  char *arg2 = strtok(NULL, " ");
  if (arg1 == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  int n = strtol(arg1, NULL, 10);
  vaddr_t expr = strtol(arg2, NULL, 16);

  int i, j;
  word_t vaddr_read();
  for (i = 0; i < n;) {
    printf("%#018x: ", expr);
    
    for (j = 0; i < n && j < 4; i++, j++) {
      word_t w = vaddr_read(expr,1);
      expr += 8;
      printf("%#08x ", w);
    }
    puts("");
  }
  
  return 0;
}

static int cmd_info(char *args) {
  if (strcmp(args, "r") == 0){
    isa_reg_display();
  }
  else if (strcmp(args, "w") == 0)
  {
    return 0;
  }
  else{
    printf("no such command");
  }
  return 0;
}

static int cmd_p(char *args){
  char *arg = strtok(NULL," ");
  if(arg == NULL){
    printf("Usage: p expr");
    return 0;
  }
  bool success = true;
  word_t ret = expr(arg,&success);
  if (success == true)
  {
    printf("10: %16d \n16:%#16x\n",ret,ret);
  }
  else{
    printf("can not identify\n");
  }
  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "execute program step by step with si follow a num,such as si 10(si [N])", cmd_si },
  { "info", "print your reg with info_r ,", cmd_info },
  { "x", "Scan Memory with (x num address)", cmd_x },
  {"p", "Usage: p EXPR. Calculate the expression, e.g. p $eax + 1", cmd_p },
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
