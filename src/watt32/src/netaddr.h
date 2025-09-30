/*!\file netaddr.h
 */
#ifndef _w32_NETADDR_H
#define _w32_NETADDR_H

#define aton            NAMESPACE (aton)
#define aton_dotless    NAMESPACE (aton_dotless)
#define isaddr          NAMESPACE (isaddr)
#define isaddr_dotless  NAMESPACE (isaddr_dotless)
#define priv_addr       NAMESPACE (priv_addr)
#define _inet_ntoa      NAMESPACE (_inet_ntoa)
#define _inet_addr      NAMESPACE (_inet_addr)
#define _inet_atoeth    NAMESPACE (_inet_atoeth)

W32_FUNC DWORD aton      (const char *str);
W32_FUNC BOOL  isaddr    (const char *text);
W32_FUNC int   priv_addr (DWORD ip);

W32_FUNC DWORD aton_dotless   (const char *str);
W32_FUNC BOOL  isaddr_dotless (const char *str, DWORD *ip);

W32_FUNC char *_inet_ntoa (char *s, DWORD x);
W32_FUNC DWORD _inet_addr (const char *s);

W32_FUNC const char *_inet_atoeth (const char *src, eth_address *eth);

W32_FUNC const char        *_inet6_ntoa (const void *ip);
W32_FUNC const ip6_address *_inet6_addr (const char *str);

#endif
