/*!\file printk.h
 */
#ifndef _w32_PRINTK_H
#define _w32_PRINTK_H

#include <stdio.h>
#include <stdarg.h>

#if defined(__DJGPP__) || defined(__HIGHC__) || defined(__WATCOMC__) || defined(__DMC__)
#undef _WIN32       /* Needed for __DMC__ */
#include <unistd.h>
#endif

#if defined(__TURBOC__) || defined(_MSC_VER) || defined(__WATCOMC__) || \
    defined(__DMC__) || defined(__MINGW32__) || defined(__LCC__)
#include <process.h>
#endif

#ifdef _w32_WATTCP_H  /* if included after wattcp.h (Watt-32 compile) */
  #define _printk_safe   NAMESPACE (_printk_safe)
  #define _printk_file   NAMESPACE (_printk_file)
  #define _printk_init   NAMESPACE (_printk_init)
  #define _printk_flush  NAMESPACE (_printk_flush)
  #define _printk        NAMESPACE (_printk)
  #define _fputsk        NAMESPACE (_fputsk)
  #define _snprintk      NAMESPACE (_snprintk)
  #define _vsnprintk     NAMESPACE (_vsnprintk)
#endif

extern int   _printk_safe;  /*!< safe to print; we're not in a RMCB/ISR. */
extern FILE *_printk_file;  /*!< what file to print to (or stdout/stderr). */

extern int   _printk_init  (int size, const char *file);
extern void  _printk_flush (void);

extern int   _fputsk    (const char *buf, FILE *stream);
extern int   _vsnprintk (char *buf, int len, const char *fmt, va_list ap);

extern int MS_CDECL _printk (const char *fmt, ...)
                     ATTR_PRINTF (1,2);

extern int MS_CDECL _snprintk (char *buf, int len, const char *fmt, ...)
                     ATTR_PRINTF (3,4);

#endif  /* _w32_PRINTK_H */

