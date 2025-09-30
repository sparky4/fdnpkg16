/*!\file udp_rev.c
 * Reverse DNS lookup
 */

/*
 *  Reverse IPv4/IPv6 lookup functions
 *
 *  Copyright (c) 1997-2002 Gisle Vanem <giva@bgnett.no>
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
 *  \version 0.2
 *    \date 14-01-2003
 *    \author Yves-Ferrant@t-online.de, Bug-fix, see read_response() below.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wattcp.h"
#include "strings.h"
#include "misc.h"
#include "timer.h"
#include "language.h"
#include "netaddr.h"
#include "pctcp.h"
#include "pcconfig.h"
#include "idna.h"
#include "bsdname.h"
#include "udp_dom.h"

/**
 * Fill in the reverse lookup question packet.
 * Return total length of request.
 */
static size_t query_init_ip4 (struct DNS_query *q, DWORD ip)
{
  char *c;
  BYTE  i, x;

  q->head.ident   = (WORD) set_timeout (0UL);  /* Random ID */
  q->head.flags   = intel16 (DRD);             /* Query, Recursion desired */
  q->head.qdcount = intel16 (1);
  q->head.ancount = 0;
  q->head.nscount = 0;
  q->head.arcount = 0;

  c  = (char*) &q->body[0];
  ip = ntohl (ip);
  for (i = 0; i < sizeof(ip); i++)
  {
    x = (BYTE) ip;
    ip >>= 8;
    *c = (x < 10) ? 1 : (x < 100) ? 2 : 3;
    itoa (x, c+1, 10);
    c += *c + 1;
  }
  strcpy (c, "\7in-addr\4arpa");
  c += 14;
  *(WORD*) c = intel16 (DTYPE_PTR);
  c += sizeof(WORD);
  *(WORD*) c = intel16 (DIN);
  c += sizeof(WORD);

  return (c - (char*)q);
}

/**
 * Read answer and extract host name.
 *   \retval 0 on error (dom_errno = DNS_CLI_ILL_RESP),
 *   \retval 1 on success.
 */

/*
Here are the decompiled data of the DNS-answer datagramm
DNS_head  CC CB       ID
          85 80       DQR | Opcode |AA|TC|RD|RA| Z | DRCODE |  (flags)
          00 01       qdcount
          00 01       ancount
          00 00       nscount
          00 00       arcount
          01 32 03 31 32 38 03 31 32 38 03 31 32 38 07 69 6E 2D  .2.128.128.128.in-
          61 64 64 72 04 61 72 70 61 00                          addr.arpa.
qtype     00 0C
qclass    00 01

Answer    01 32 03 31 32 38 03 31 32 38 03 31 32 38 07 69        .2.128.128.128.i
          6E 2D 61 64 64 72 04 61 72 70 61 00                    n-addr.arpa.
qtype     00 0C
qclass    00 01
TTL       00 00 0E 10
Length    00 10
Answer    05 6C 69 6E 75 78 04 79 76 65 73 03 6E 65 74 00        .linux.yves.net

Yves-Ferrant noted that if DNS server does *not* use message compression,
this function failed to find the answer.
Ref. RFC1035, Section 4.1.4. Message compression

*/

static BOOL read_response (sock_type *s, char *name, size_t size)
{
  struct DNS_query            response;
  const  struct DNS_resource *rr;
  const  char *name_end;
  const  BYTE *c, *resp_end;
  size_t len;
  BYTE   rcode;
  BOOL   compressed;

  memset (&response, 0, sizeof(response));
  len   = sock_fastread (s, (BYTE*)&response, sizeof(response));
  rcode = (DRCODE & intel16(response.head.flags));

  if (len < sizeof(response.head) || response.head.qdcount != intel16(1))
     goto fail;

  if (rcode != DNS_SRV_OK || response.head.ancount == 0)
     goto no_name;

  resp_end   = (const BYTE*)&response + len;
  name_end   = name + size;
  compressed = FALSE;

  /* Skip question */
  c = response.body;
  while (*c && c < resp_end)
        c++;

  c += 5;          /* skip QNAME, QTYPE and QCLASS */
  if (c >= resp_end)
     goto fail;

  /* Skip name */
  while (*c && c < resp_end)
  {
    if (*c >= 0xC0)  /* compressed result */
    {
      c += 2;
      compressed = TRUE;
      break;
    }
    c++;
  }
  if (!compressed)
     c++;

  if (c + RESOURCE_HEAD_SIZE >= resp_end)
     goto fail;

  rr = (const struct DNS_resource*) c;

  if (rr->rtype != intel16(DTYPE_PTR))
     goto fail;

  c = rr->rdata;
  dom_ttl = intel (rr->ttl);

  while (*c && c < resp_end)  /* will truncate name if c >= resp_end */
  {
    len = *c;
    if (len >= 0xC0)
    {
      WORD ofs = (len - 0xC0) + c[1];
      c = response.body + ofs;
      len = *c;
    }
    if (name + len >= name_end-1)
       break;
    strncpy (name, (const char*)(c+1), len);
    name += len;
    c += len + 1;
    if (*c)
       *name++ = '.';
  }
  *name = '\0';

#if defined(USE_IDNA)
  if (dns_do_idna && !IDNA_convert_from_ACE(name,&size))
  {
    dom_errno = DNS_CLI_ILL_IDNA;
    return (FALSE);
  }
#endif

  dom_errno = DNS_SRV_OK;
  return (TRUE);

fail:
  dom_errno = DNS_CLI_ILL_RESP;
  return (FALSE);

no_name:
  dom_errno = rcode;
  return (FALSE);
}

/**
 * Translate an IPv4/IPv6 address into a host name.
 * Returns 1 on success, 0 on error or timeout.
 */
static BOOL reverse_lookup (const struct DNS_query *q, size_t qlen,
                            char *name, size_t size, DWORD nameserver)
{
  BOOL        ret;
  BOOL        ready = FALSE;
  BOOL        quit  = FALSE;
  unsigned    sec;
  DWORD       timer;
  _udp_Socket dom_sock;
  sock_type  *sock = NULL;

  if (!nameserver)      /* no nameserver, give up */
  {
    dom_errno = DNS_CLI_NOSERV;
    outsnl (dom_strerror(dom_errno));
    return (FALSE);
  }

  if (!udp_open(&dom_sock, DOM_SRC_PORT, nameserver, DOM_DST_PORT, NULL))
  {
    dom_errno = DNS_CLI_SYSTEM;
    return (FALSE);
  }

  timer = set_timeout (1000 * dns_timeout);

  for (sec = 2; sec < dns_timeout-1 && !quit && !_resolve_exit; sec *= 2)
  {
    sock = (sock_type*)&dom_sock;
    sock_write (sock, (const BYTE*)q, qlen);
    ip_timer_init (sock, sec);

    do
    {
      tcp_tick (sock);

      if (_watt_cbroke || (_resolve_hook && (*_resolve_hook)() == 0))
      {
        _resolve_exit = 1;       /* terminate do_rresolve() */
        dom_errno = DNS_CLI_USERQUIT;
        quit  = TRUE;
        ready = FALSE;
        break;
      }
      if (sock_dataready(sock))
      {
        quit  = TRUE;
        ready = TRUE;
      }
      if (ip_timer_expired(sock) || chk_timeout(timer))
      {
        dom_errno = DNS_CLI_TIMEOUT;
        ready = FALSE;
        _resolve_timeout = 1;
        break;  /* retry */
      }
      if (sock->udp.usr_yield)
           (*sock->udp.usr_yield)();
      else WATT_YIELD();
    }
    while (!quit);
  }

  if (ready && sock)
       ret = read_response (sock, name, size);
  else ret = FALSE;

  if (sock)  /* if we ran the above for-loop */
     sock_close (sock);
  return (ret);
}

/**
 * Do a reverse lookup on `my_ip_addr'. If successfull, replace
 * `hostname' and `def_domain' with returned result.
 */
int reverse_lookup_myip (void)
{
  char name [MAX_HOSTLEN];

  if (!reverse_resolve_ip4(htonl(my_ip_addr),name,sizeof(name)))
     return (0);

  if (debug_on >= 1)
  {
    outs (_LANG("My FQDN: "));
    outsnl (name);
  }
  if (sethostname(name,sizeof(name)) < 0)
     return (0);
  return (1);
}

/*------------------------------------------------------------------*/

static int do_rresolve (const struct DNS_query *q, size_t qlen,
                        char *result, size_t size)
{
  WORD brk_mode;
  int  i;

  if (dns_timeout == 0)
      dns_timeout = (UINT)sock_delay << 2;

  NEW_BREAK_MODE (brk_mode, 1);

  _resolve_exit = _resolve_timeout = 0;

  *result = '\0';
  dom_cname[0] = '\0';

  for (i = 0; i < last_nameserver; ++i)
      if (reverse_lookup(q,qlen,result,size,def_nameservers[i]) ||
          _resolve_exit)
         break;

  OLD_BREAK_MODE (brk_mode);
  return (*result != '\0');
}

/*------------------------------------------------------------------*/

int reverse_resolve_ip4 (DWORD ip, char *result, size_t size)
{
  struct DNS_query q;
  size_t len = query_init_ip4 (&q, ip);

  memset (&dom_a4list, 0, sizeof(dom_a4list));

#if defined(WIN32) && defined(HAVE_WINDNS_H)
  if (WinDnsQuery_PTR4(ip, result, size))
     return (1);
#endif
  return do_rresolve (&q, len, result, size);
}

#if defined(USE_IPV6)

#define USE_IP6_BITSTRING 0     /* RFC-2673 */

/**
 * Fill in the reverse lookup question packet.
 * Return total length of request.
 * \todo Use "ip6.arpa" bitstring format ?
 */
static size_t query_init_ip6 (struct DNS_query *q, const void *addr)
{
  const BYTE *ip = (const BYTE*) addr;
  char *c;
  int   i;

  q->head.ident   = (WORD) set_timeout (0UL);  /* Random ID */
  q->head.flags   = intel16 (DRD);             /* Query, Recursion desired */
  q->head.qdcount = intel16 (1);               /* 1 query */
  q->head.ancount = 0;
  q->head.nscount = 0;
  q->head.arcount = 0;

  c = (char*) &q->body[0];

#if USE_IP6_BITSTRING
  strcpy (c, "\3\\[x");
  c += 4;
  for (i = 0; i < SIZEOF(ip6_address); i++)
  {
    int hi = ip[i] >> 4;
    int lo = ip[i] & 15;

    if (i == SIZEOF(ip6_address) - 1)
         *c++ = 5;
    else *c++ = 4;
    *c++ = hex_chars_lower [hi];
    *c++ = hex_chars_lower [lo];
  }
  strcpy (c, "]\3ip6\4arpa");
  c += 11;

#else
  for (i = sizeof(ip6_address)-1; i >= 0; i--)
  {
    int hi = ip[i] >> 4;
    int lo = ip[i] & 15;

    *c++ = 2;
    *c++ = hex_chars_lower [hi];
    *c++ = hex_chars_lower [lo];
  }
  strcpy (c, "\3ip6\4arpa");
  c += 10;
#endif

  *(WORD*) c = intel16 (DTYPE_PTR);
  c += sizeof(WORD);
  *(WORD*) c = intel16 (DIN);
  c += sizeof(WORD);

  return (c - (char*)q);
}

int reverse_resolve_ip6 (const void *addr, char *result, size_t size)
{
  struct DNS_query q;
  size_t len = query_init_ip6 (&q, addr);

  memset (&dom_a6list, 0, sizeof(dom_a6list));
#if defined(WIN32) && defined(HAVE_WINDNS_H)
  if (WinDnsQuery_PTR6(addr, result, size))
     return (1);
#endif
  return do_rresolve (&q, len, result, size);
}
#endif   /* USE_IPV6 */

