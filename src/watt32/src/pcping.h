/*!\file pcping.h
 */
#ifndef _w32_PCPING_H
#define _w32_PCPING_H

W32_FUNC int   _ping     (DWORD host, DWORD count, const BYTE *patt, size_t len);
W32_FUNC DWORD _chk_ping (DWORD host, DWORD *number);
W32_FUNC void  add_ping  (DWORD host, DWORD tim, DWORD number);

#endif
