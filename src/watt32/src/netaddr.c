/*!\file netaddr.c
 *
 *  Provide some more BSD address functions.
 *  Gisle Vanem, Oct 12, 1995
 *
 *  inet_network() is Copyright (c) 1983, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 *  \version v0.0
 *    \date Jan 11, 1991 \author E. Engelke
 *
 *  \version v0.2
 *    \date Apr 10, 1995 \author G. Vanem, function priv_addr()
 *
 *  \version v0.3
 *    \date Nov 12, 2003 \author G. Vanem, some functions moved from now
 *                       obsolete udp_nds.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "wattcp.h"
#include "misc.h"
#include "strings.h"
#include "netaddr.h"

#if defined(USE_BSD_API)
  #include <arpa/inet.h>
  #include <sys/socket.h>
#endif

/**
 * Convert an IP-address 'ip' into a string.
 *  \retval 's'.
 *  \note 'ip' is on \b host order.
 */
char *_inet_ntoa (char *s, DWORD ip)
{
  static char buf[2][20];
  static int idx = 0;

  if (!s)
  {
    s = buf [idx];
    idx ^= 1;
  }
  itoa ((int)(ip >> 24), s, 10);
  strcat (s, ".");
  itoa ((int)(ip >> 16) & 0xFF, strchr(s,'\0'), 10);
  strcat (s, ".");
  itoa ((int)(ip >> 8) & 0xFF, strchr(s,'\0'), 10);
  strcat (s, ".");
  itoa ((int)(ip & 0xFF), strchr(s,'\0'), 10);
  return (s);
}

/**
 * Convert a string into an IP-address.
 *  \retval address on \b host order.
 *  \retval 0 string isn't a (dotless) address.
 */
DWORD _inet_addr (const char *s)
{
  DWORD addr = 0;

  if (isaddr(s))
     return aton (s);
  if (isaddr_dotless(s,&addr))
     return (addr);
  return (0);
}


/**
 * Converts [a.b.c.d] or a.b.c.d to 32 bit IPv4 address.
 *  \retval 0 on error (safer than -1)
 *  \retval IPv4 address (host order)
 */
DWORD aton (const char *str)
{
  DWORD ip = 0;
  int   i;
  char *s = (char*)str;

  if (*s == '[')
     ++s;

  for (i = 24; i >= 0; i -= 8)
  {
    int cur = ATOI (s);

    ip |= (DWORD)(cur & 0xFF) << i;
    if (!i)
       return (ip);

    s = strchr (s, '.');
    if (!s)
       return (0);      /* return 0 on error */
    s++;
  }
  return (0);           /* cannot happen ??  */
}

/**
 * Converts `[dotless]' or `dotless' to 32 bit long (host order)
 *  \retval 0 on error (safer than -1)
 */
DWORD aton_dotless (const char *str)
{
  DWORD ip = 0UL;

  isaddr_dotless (str, &ip);
  return (ip);
}

/**
 * Check if 'str' is simply an ip address.
 * Accepts 'a.b.c.d' or '[a.b.c.d]' forms.
 *  \retval TRUE if 'str' is an IPv4 address.
 */
BOOL isaddr (const char *str)
{
  int ch;

  while ((ch = *str++) != 0)
  {
    if (isdigit(ch))
       continue;
    if (ch == '.' || ch == ' ' || ch == '[' || ch == ']')
       continue;
    return (FALSE);
  }
  return (TRUE);
}

/**
 * Check if 'str' is a dotless ip address.
 * Accept "[ 0-9\[\]]+". Doesn't accept octal base.
 *  \retval TRUE if 'str' is a dotless address.
 */
BOOL isaddr_dotless (const char *str, DWORD *ip)
{
  char  buf[10] = { 0 };
  int   ch, i = 0;
  DWORD addr;

  while ((ch = *str++) != '\0')
  {
    if (ch == '.' || i == SIZEOF(buf))
       return (FALSE);

    if (isdigit(ch))
    {
      buf[i++] = ch;
      continue;
    }
    if (ch == ' ' || ch == '[' || ch == ']')
       continue;
    return (FALSE);
  }

  buf[i] = '\0';
  addr = atol (buf);
  if (addr == 0)
     return (FALSE);

  if ((addr % 256) == 0)         /* LSB must be non-zero */
     return (FALSE);

  if (((addr >> 24) % 256) == 0) /* MSB must be non-zero */
     return (FALSE);

  if (ip)
     *ip = addr;
  return (TRUE);
}

char * W32_CALL inet_ntoa (struct in_addr addr)
{
  static char buf [4][20];   /* use max 4 at a time */
  static int  idx = 0;
  char       *rc  = buf [idx++];

  idx &= 3;
  return _inet_ntoa (rc, ntohl(addr.s_addr));
}

/**
 * Parse string "ether-addr, ip-addr".
 *
 * DOSX == 0: Assumes 'src' contain only legal hex-codes
 *   and ':' separators. Doesn't waste precious memory using
 *   sscanf() if DOSX==0.
 *
 * Read 'src', dump to ethernet buffer ('*p_eth')
 * and return pointer to "ip-addr".
 */
const char *_inet_atoeth (const char *src, eth_address *p_eth)
{
  BYTE *eth = (BYTE*)p_eth;

#if (DOSX)
  int tmp [sizeof(eth_address)];

  if (sscanf(src,"%02x:%02x:%02x:%02x:%02x:%02x",
              &tmp[0], &tmp[1], &tmp[2],
              &tmp[3], &tmp[4], &tmp[5]) != DIM(tmp))
     return (NULL);

  eth [0] = tmp [0];
  eth [1] = tmp [1];
  eth [2] = tmp [2];
  eth [3] = tmp [3];
  eth [4] = tmp [4];
  eth [5] = tmp [5];
  src = strrchr (src, ':') + 3;
#else
  eth [0] = atox (src-2);      /*   xx:xx:xx:xx:xx:xx */
  eth [1] = atox (src+1);      /* ^src-2              */
  eth [2] = atox (src+4);      /*    ^src+1           */
  eth [3] = atox (src+7);
  eth [4] = atox (src+10);
  eth [5] = atox (src+13);

  src += strlen ("xx:xx:xx:xx:xx:xx");
#endif

  if (*src == ',')
     ++src;
  return strltrim (src);
}

#if defined(USE_BSD_API)

char * W32_CALL inet_ntoa_r (struct in_addr addr, char *buf, int buflen)
{
  if (buf && buflen >= SIZEOF("255.255.255.255"))
     return _inet_ntoa (buf, ntohl(addr.s_addr));
  return (NULL);
}

int W32_CALL inet_aton (const char *name, struct in_addr *addr)
{
  u_long ip = inet_addr (name);

  if (ip == INADDR_NONE && strcmp(name,"255.255.255.255"))
     return (0);
  addr->s_addr = ip;
  return (1);
}

u_long W32_CALL inet_addr (const char *addr)
{
  DWORD ip = htonl (_inet_addr(addr));

  if (ip)
     return (ip);
  return (INADDR_NONE);
}

u_long W32_CALL inet_network (const char *name)
{
  u_long  val, base;
  u_long  parts[4];
  u_long *pp = parts;
  int     i, c, n;

again:
  /* Collect number up to ``.''. Values are specified as for C:
   * 0x=hex, 0=octal, other=decimal.
   */
  val  = 0;
  base = 10;

  /* The 4.4BSD version of this file also accepts 'x__' as a hexa
   * number.  I don't think this is correct.  -- Uli
   */
  if (*name == '0')
  {
    if (*++name == 'x' || *name == 'X')
         base = 16, name++;
    else base = 8;
  }
  while ((c = *name) != '\0')
  {
    if (isdigit(c))
    {
      val = (val * base) + (c - '0');
      name++;
      continue;
    }
    if (base == 16 && isxdigit(c))
    {
      val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
      name++;
      continue;
    }
    break;
  }
  if (*name == '.')
  {
    if (pp >= parts + 4)
       return (INADDR_NONE);

    *pp++ = val;
    name++;
    goto again;
  }
  if (*name && !isspace((int)*name))
     return (INADDR_NONE);

  *pp++ = val;
  n = pp - parts;
  if (n > 4)
     return (INADDR_NONE);

  for (val = 0, i = 0; i < n; i++)
  {
    val <<= 8;
    val |= parts[i] & 0xff;
  }
  return (val);
}

/**
 * Return the network number from an internet
 * address; handles class A/B/C network #'s.
 */
u_int32_t W32_CALL inet_netof (struct in_addr addr)
{
  u_long a = ntohl (addr.s_addr);

  if (IN_CLASSA(a))
     return ((a & IN_CLASSA_NET) >> IN_CLASSA_NSHIFT);

  if (IN_CLASSB(a))
     return ((a & IN_CLASSB_NET) >> IN_CLASSB_NSHIFT);

  return ((a & IN_CLASSC_NET) >> IN_CLASSC_NSHIFT);
}

/**
 * Return the local network address portion of an
 * internet address; handles class A/B/C network
 * number formats only.
 * \note return value is host-order.
 */
u_long W32_CALL inet_lnaof (struct in_addr addr)
{
  u_long a = ntohl (addr.s_addr);

  if (IN_CLASSA(a))
     return (a & IN_CLASSA_HOST);

  if (IN_CLASSB(a))
     return (a & IN_CLASSB_HOST);

  return (a & IN_CLASSC_HOST);
}

/**
 * Formulate an Internet address from network + host (with subnet address).
 *  \note 'net' is on host-order.
 */
struct in_addr W32_CALL inet_makeaddr (u_long net, u_long host)
{
  u_long addr;

  if (net < 128)
       addr = (net << IN_CLASSA_NSHIFT) | (host & IN_CLASSA_HOST);

  else if (net < 65536)
       addr = (net << IN_CLASSB_NSHIFT) | (host & IN_CLASSB_HOST);

  else if (net < 16777216L)
       addr = (net << IN_CLASSC_NSHIFT) | (host & IN_CLASSC_HOST);

  else addr = (net | host);

  addr = htonl (addr);
  return (*(struct in_addr*) &addr);
}

/**
 * Convert an IPv6-address 'ip' into a string.
 *  \retval  static string.
 *  \warning Not reentrant.
 */
const char *_inet6_ntoa (const void *ip)
{
  static char buf [4][50];   /* use max 4 at a time */
  static int  idx = 0;
  char  *rc = buf [idx++];

  /* inet_ntop() should never return NULL
   */
  idx &= 3;
  rc = (char*) inet_ntop (AF_INET6, ip, rc, sizeof(buf[0]));
  return (const char*)rc;
}

/**
 * Convert a string 'str' into an IPv6-address.
 *  \retval static buffer for address.
 *  \retval NULL 'str' isn't a valid IPv6 address.
 */
const ip6_address *_inet6_addr (const char *str)
{
  static ip6_address ip6;

  if (!inet_pton(AF_INET6, str, &ip6))
     return (NULL);
  return (const ip6_address*)ip6;
}
#endif  /* USE_BSD_API */


#ifdef NOT_USED
BOOL priv_addr (DWORD ip)
{
  /*
   * private addresses in the "blackhole range" (ref. RFC-1918):
   *   10.0.0.0    - 10.255.255.255
   *   172.16.0.0  - 172.31.255.255
   *   192.168.0.0 - 192.168.255.255
   */
  if (ip >= aton("10.0.0.0") && ip <= aton("10.255.255.255"))
     return (TRUE);

  if (ip >= aton("172.16.0.0") && ip <= aton("172.31.255.255"))
     return (TRUE);

  if (ip >= aton("192.168.0.0") && ip <= aton("192.168.255.255"))
     return (TRUE);

  return (FALSE);
}

/*
 * Lev Walkin <vlm@lionet.info> provided these handy mask-checking
 * macros/routines.
 */

/* Masklen to mask convertor.
 */
#define MLEN2MASK(mlen) ((mlen) ? (((DWORD)-1) << (32 - (mlen))) : 0)

/* Determine number of bits set in an IP mask.
 */
#define BITSSET(v) ( {                                         \
        DWORD __n = v;                                         \
        __n = (__n & 0x55555555) + ((__n & 0xaaaaaaaa) >> 1);  \
        __n = (__n & 0x33333333) + ((__n & 0xcccccccc) >> 2);  \
        __n = (__n & 0x0f0f0f0f) + ((__n & 0xf0f0f0f0) >> 4);  \
        __n = (__n & 0x00ff00ff) + ((__n & 0xff00ff00) >> 8);  \
        __n = (__n & 0x0000ffff) + ((__n & 0xffff0000) >> 16); \
        __n; } )

/*
 * Check the mask for correctness.
 */
int check_mask (DWORD mask)
{
  return MLEN2MASK (BITSSET(mask)) == mask;
}

/*
 * The string-based version of above function.
 */
int check_mask2 (const char *mask)
{
  struct in_addr imask;

  if (inet_aton(mask,&imask) != 1)
     return (-1);
  return check_mask (imask.s_addr);
}
#endif


