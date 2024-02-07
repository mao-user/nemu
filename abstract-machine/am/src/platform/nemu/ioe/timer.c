#include <am.h>
#include <nemu.h>

void __am_timer_init() {
  // i8253计时器会注册长度为8字节的MMIO空间, 又riscv.h中最大处理数据为32位, 所以分
  // 高4字节和低4字节. 
  outl(RTC_ADDR, 0);        
  outl(RTC_ADDR + 4, 0);
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  uptime->us = inl(RTC_ADDR + 4);
  uptime->us <<= 32;
  uptime->us += inl(RTC_ADDR);
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
