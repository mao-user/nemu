#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int vsprintf(char *out, const char *fmt, va_list ap);
int vsnprintf(char *out, size_t n, const char *fmt, va_list ap);

int printf(const char *fmt, ...) {
	char buffer[2048];
	va_list arg;
	va_start (arg, fmt);
	int done = vsprintf(buffer, fmt, arg);
	putstr(buffer);
	va_end(arg);
	return done;
}

static char HEX_CHARACTERS[] = "0123456789ABCDEF";
#define BIT_WIDE_HEX 8

static void reverse(char *s, int len) {
  char *end = s + len - 1;
  char tmp;
  while (s < end) {
    tmp = *s;
    *s = *end;
    *end = tmp;
  }
}

static int itoa(int n, char *s, int base) {
  assert(base <= 16);

  int i = 0, sign = n, bit;
  if (sign < 0) n = -n;
  do {
    bit = n % base;
    if (bit >= 10) s[i++] = 'a' + bit - 10;
    else s[i++] = '0' + bit;
  } while ((n /= base) > 0);
  if (sign < 0) s[i++] = '-';
  s[i] = '\0';
  reverse(s, i);

  return i;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  char *start = out;
  
  for (; *fmt != '\0'; ++fmt) {
    if (*fmt != '%') {
      *out = *fmt;
      ++out;
    } else {
      switch (*(++fmt)) {
      case '%': *out = *fmt; ++out; break;
      case 'd': out += itoa(va_arg(ap, int), out, 10); break;
      case 's':
        char *s = va_arg(ap, char*);
        strcpy(out, s);
        out += strlen(out);
        break;
      }
    }
  }
  
  *out = '\0';
  return out - start;
}

int sprintf(char *out, const char *fmt, ...) {
	va_list valist;
	va_start(valist, fmt);
	int res = vsprintf(out ,fmt, valist);
	va_end(valist);
	return res;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
	va_list arg;
	va_start (arg, fmt);
	int done = vsnprintf(out, n, fmt, arg);
	va_end(arg);
	return done;
}

#define append(x) {out[j++]=x; if (j >= n) {break;}}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
	char buffer[128];
	char *txt, cha;
	int num, len;
	unsigned int unum;
	uint32_t pointer;
	int state = 0, i, j;
	for (i = 0, j = 0; fmt[i] != '\0'; ++i){
		switch (state){
		/*
		* this state is used to copy the contents of the 
		* string to the buffer 
		*/
		case 0:
			if (fmt[i] != '%'){
				append(fmt[i]);
			} 
			else
				state = 1;
			break;
		/*
		* this state is used to deal with the format controler  
		*/
		case 1:
			switch (fmt[i]){
				case 's':
					txt = va_arg(ap, char*);
					for (int k = 0; txt[k] !='\0'; ++k)
						append(txt[k]);
					break;
				
				case 'd':
					num = va_arg(ap, int);
					if(num == 0){
						append('0');
						break;
					}
					if (num < 0){
						append('-');
						num = 0 - num;
					}
					for (len = 0; num ; num /= 10, ++len)
						buffer[len] = HEX_CHARACTERS[num % 10];
					for (int k = len - 1; k >= 0; --k)
						append(buffer[k]);
					break;
				
				case 'c':
					cha = (char)va_arg(ap, int);
					append(cha);
					break;

				case 'p':
					pointer = va_arg(ap, uint32_t);
					for (len = 0; pointer ; pointer /= 16, ++len)
						buffer[len] = HEX_CHARACTERS[pointer % 16];
					for (int k = 0; k < BIT_WIDE_HEX - len; ++k)
						append('0');
					for (int k = len - 1; k >= 0; --k)
						append(buffer[k]);
					break;
				case 'x':
					unum = va_arg(ap, unsigned int);
					if(unum == 0){
						append('0');
						break;
					}
					for (len = 0; unum ; unum >>= 4, ++len)
						buffer[len] = HEX_CHARACTERS[unum & 0xF];
					for (int k = len - 1; k >= 0; --k)
						append(buffer[k]);
					break;  
				default:
					assert(0);
			}
		state = 0;
		break;
		}
	}
	out[j] = '\0';
	return j;
}

#endif
