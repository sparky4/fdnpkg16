/*
 * The following is a macro-like function that uses kitten instead of directly doing printf
 */

#include <stdio.h>    /* vprintf() */
#include <stdarg.h>   /* va_list, va_start()... */

#include "kittendj.h" /* Kitten headers */
#include "kprintf.h"
#include "version.h"

void kitten_printf(short x, short y, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(kittengets(x,y,fmt), args);
  va_end(args);
}

void kitten_puts(short x, short y, char *fmt) {
  puts(kittengets(x,y,fmt));
}
