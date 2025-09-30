/*!\file bsdname.h
 */
#ifndef _w32_BSDNAME_H
#define _w32_BSDNAME_H

#ifndef __SYS_SOCKET_H
#include <sys/socket.h>
#endif

/* Core-style name functions. BSD-style in above header.
 */
W32_FUNC int   _getpeername (const sock_type *s, void *dest, int *len);
W32_FUNC int   _getsockname (const sock_type *s, void *dest, int *len);
W32_FUNC DWORD _gethostid   (void);
W32_FUNC DWORD _sethostid   (DWORD ip);

/* privates */
extern int         _get_machine_name (char *buf, int size);
extern void        _sethostid6 (const void *addr);
extern const void *_gethostid6 (void);

#endif
