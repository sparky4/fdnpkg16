/*!\file getput.c
 *
 *  get/put short/long functions for little-endian platforms.
 */

/*  Copyright (c) 1997-2002 Gisle Vanem <giva@bgnett.no>
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. All advertising materials mentioning features or use of this software
 *     must display the following acknowledgement:
 *       This product includes software developed by Gisle Vanem
 *       Bergen, Norway.
 *
 *  THIS SOFTWARE IS PROVIDED BY ME (Gisle Vanem) AND CONTRIBUTORS ``AS IS''
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL I OR CONTRIBUTORS BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "wattcp.h"
#include "chksum.h"
#include "misc.h"

/*
 * Functions for get/put short/long. Pointer is _NOT_ advanced.
 */
#if !defined(USE_BIGENDIAN)

#define GETSHORT(s, cp) {            \
        BYTE *t_cp = (BYTE*)(cp);    \
        (s) = ((WORD)t_cp[0] << 8)   \
            | ((WORD)t_cp[1]);       \
      }

#define GETLONG(l, cp) {             \
        BYTE *t_cp = (BYTE*)(cp);    \
        (l) = ((DWORD)t_cp[0] << 24) \
            | ((DWORD)t_cp[1] << 16) \
            | ((DWORD)t_cp[2] << 8)  \
            | ((DWORD)t_cp[3]);      \
      }

#define PUTSHORT(s, cp) {            \
        WORD  t_s  = (WORD)(s);      \
        BYTE *t_cp = (BYTE*)(cp);    \
        *t_cp++ = (BYTE)(t_s >> 8);  \
        *t_cp   = (BYTE)t_s;         \
      }

#define PUTLONG(l, cp) {             \
        DWORD t_l  = (DWORD)(l);     \
        BYTE *t_cp = (BYTE*)(cp);    \
        *t_cp++ = (BYTE)(t_l >> 24); \
        *t_cp++ = (BYTE)(t_l >> 16); \
        *t_cp++ = (BYTE)(t_l >> 8);  \
        *t_cp   = (BYTE)t_l;         \
      }

WORD _getshort (const BYTE *x)   /* in <arpa/nameserv.h> */
{
  WORD res;
  GETSHORT (res, x);
  return (res);
}

DWORD _getlong (const BYTE *x)   /* in <arpa/nameserv.h> */
{
  DWORD res;
  GETLONG (res, x);
  return (res);
}

void __putshort (WORD var, BYTE *ptr)   /* in <resolv.h> */
{
  PUTSHORT (var, ptr);
}

void __putlong (DWORD var, BYTE *ptr)   /* in <resolv.h> */
{
  PUTLONG (var, ptr);
}

/*
 * If compiler/linker doesn't see our defines for htonl() etc.
 */
#undef htonl
#undef ntohl
#undef htons
#undef ntohs

DWORD W32_CALL htonl (DWORD val)
{
  return intel (val);
}
DWORD W32_CALL ntohl (DWORD val)
{
  return intel (val);
}
WORD W32_CALL htons (WORD val)
{
  return intel16 (val);
}
WORD W32_CALL ntohs (WORD val)
{
  return intel16 (val);
}
#endif  /* USE_BIGENDIAN */
