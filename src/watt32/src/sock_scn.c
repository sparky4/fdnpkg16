/*!\file sock_scn.c
 *
 * scanf-like function for sock_type.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "copyrigh.h"
#include "wattcp.h"
#include "misc.h"
#include "pctcp.h"

/*
 * MSC/DMC/DJGPP 2.01 doesn't have vsscanf().
 * On Windows there's nothing to be done :-(
 */
#if defined(WIN32) && !defined(__MINGW32__)
  static int vsscanf (const char *buf, const char *fmt, va_list arglist)
  {
    ARGSUSED (buf);
    ARGSUSED (fmt);
    ARGSUSED (arglist);
    return (0);
  }
#elif defined(__DJGPP__) && (DJGPP_MINOR < 2)
  static int vsscanf (const char *buf, const char *fmt, va_list arglist)
  {
    FILE *fil = stdin;
    fil->_ptr = (char*) buf;
    return _doscan (fil, fmt, arglist);
  }
#elif defined(_MSC_VER) || defined(__DMC__)
  static int vsscanf (const char *buf, const char *fmt, va_list arglist)
  {
    extern _input (FILE*, const char*, va_list);
    FILE *fil = stdin;
    fil->_ptr = (char*) buf;
    return _input (fil, fmt, arglist);
  }
#endif

/*
 * sock_scanf - Read a line and return number of fields matched.
 *
 * BIG WARNING: Don't use this for packetised protocols like
 *              SSH. Only suitable for ASCII orientented protocols
 *              like POP3/SMTP/NNTP etc.
 *
 * NOTE: Don't use {\r\n} in 'fmt' (it's stripped in sock_gets).
 */
int MS_CDECL sock_scanf (sock_type *s, const char *fmt, ...)
{
  char buf [tcp_MaxBufSize];
  int  fields = 0;
  int  status, len;

  while ((status = sock_dataready(s)) == 0)
  {
    if (status == -1)
       return (-1);

    len = sock_gets (s, (BYTE*)buf, sizeof(buf));
    if (len > 0)
    {
      va_list args;
      va_start (args, fmt);
      fields = vsscanf (buf, fmt, args);
      va_end (args);
      break;
    }
  }
  return (fields);
}

