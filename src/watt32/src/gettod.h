/*!\file gettod.h
 */
#ifndef _w32_GETTIMEOFDAY_H
#define _w32_GETTIMEOFDAY_H

#include <sys/wtime.h>

#if (DOSX) && defined(HAVE_UINT64)
extern BOOL get_tv_from_tsc (const struct ulong_long *tsc,
                             struct timeval *tv);
#endif

extern void set_utc_offset (void);
extern int  gettimeofday2 (struct timeval *tv, struct timezone *tz);

#endif
