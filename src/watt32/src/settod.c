/*!\file settod.c
 *
 *  settimeofday() for non-djgpp targets.
 *
 *  The exported prototype used is used is the one specified in the
 *  XOpen/POSIX 1.3 standards and the one used on modern (ie 4.4BSD spec)
 *  BSDs. ie 'int settimeofday(struct timeval *, ...)', ie the second
 *  arg, if specified, is ignored.
 *
 *  Cyrus Patel <cyp@fb14.uni-mainz.de>
 *
 *  May 2001 - Created
 *
 */

#if !defined(__DJGPP__)  /* allready have settimeofday() */

#include <errno.h>       /* EINVAL */
#include <sys/wtime.h>   /* time func/types */

#include "wattcp.h"
#include "misc.h"
#include "settod.h"

int MS_CDECL settimeofday (struct timeval *tv, ...)
{
  if (tv)
  {
    time_t t       = (time_t)tv->tv_sec;
    struct tm *tmp = localtime (&t);

    if (tmp && tmp->tm_year >= 80)
    {
#if defined(WIN32)
      /* todo */
#else
      struct dosdate_t newdate;
      struct dostime_t newtime;

      newdate.year    = (WORD)(tmp->tm_year + 1900);
      newdate.month   = (BYTE)(tmp->tm_mon + 1);
      newdate.day     = (BYTE)(tmp->tm_mday);
      newtime.hour    = (BYTE)(tmp->tm_hour);
      newtime.minute  = (BYTE)(tmp->tm_min);
      newtime.second  = (BYTE)(tmp->tm_sec);
      newtime.hsecond = (BYTE)(tv->tv_usec / 10000ul);
      if (_dos_setdate(&newdate) == 0  && /* int 21h fxn 2Bh */
          _dos_settime(&newtime) == 0)    /* int 21h fxn 2Dh */
        return (0);
#endif
    }
  }
  SOCK_ERRNO (EINVAL);
  return (-1);
}
#endif /* !__DJGPP__ */
