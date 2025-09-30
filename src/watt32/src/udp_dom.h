/*!\file udp_dom.h
 */
#ifndef _w32_UDP_DOM_H
#define _w32_UDP_DOM_H

#define MAX_NAMESERVERS   10  /* max # of nameservers supported (plenty!) */
#define MAX_LABEL_SIZE    63  /* maximum length of a single domain label  */
#define DOMSIZE          512  /* maximum domain message size to mess with */
#define DOM_DST_PORT      53  /* destination port number for DNS protocol */
#define DOM_SRC_PORT    1415  /* local port number for DNS protocol. Old  */
                              /* port 997 didn't work with some firewalls */
#include <sys/packon.h>

/*
 * Header format (struct DNS_head), from RFC 1035:
 *                                  1  1  1  1  1  1
 *    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                      ID                       |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    QDCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    ANCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    NSCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    ARCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 * AA, TC, RA, and RCODE are only set in responses.
 * Brief description of the remaining fields:
 *      ID      Identifier to match responses with queries
 *      QR      Query (0) or response (1)
 *      Opcode  For our purposes, always QUERY
 *      RD      Recursion desired
 *      Z       Reserved (zero)
 *      QDCOUNT Number of queries
 *      ANCOUNT Number of answers
 *      NSCOUNT Number of name server records
 *      ARCOUNT Number of additional records
 *
 * Question format, from RFC 1035:
 *                                  1  1  1  1  1  1
 *    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                                               |
 *  /                     QNAME                     /
 *  /                                               /
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                     QTYPE                     |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                     QCLASS                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 * The QNAME is encoded as a series of labels, each represented
 * as a one-byte length (maximum 63) followed by the text of the
 * label. The list is terminated by a label of length zero (which can
 * be thought of as the root domain).
 */

/*
 * Header for the DOMAIN queries.
 * ALL OF THESE ARE BYTE SWAPPED QUANTITIES!
 * We are the poor slobs who are incompatible with the world's byte order.
 */
struct DNS_head {
       WORD  ident;          /* Unique identifier              */
#if 1
       WORD  flags;          /* See below                      */
#else
       unsigned rd     : 1;    /* Recursion desired */
       unsigned tc     : 1;    /* Truncated message */
       unsigned aa     : 1;    /* Authoritative answer */
       unsigned opcode : 4;    /* See below */
       unsigned qr     : 1;    /* Response = 1, Query = 0 */
       unsigned rcode  : 4;    /* Response code */
       unsigned cd     : 1;    /* Checking disabled by resolver */
       unsigned ad     : 1;    /* Authentic data from named */
       unsigned unused : 1;    /* Unused bits */
       unsigned ra     : 1;    /* Recursion available */
#endif
       WORD  qdcount;        /* Question section, # of entries */
       WORD  ancount;        /* Answers, how many              */
       WORD  nscount;        /* Count of name server RRs       */
       WORD  arcount;        /* Number of "additional" records */
     };

/*
 * DNS_head::flags
 */
#define DQR       0x8000     /* Query = 0, Response = 1 */
#define DOPCODE   0x7100     /* Opcode mask, see below  */
#define DAA       0x0400     /* Authoritative answer    */
#define DTC       0x0200     /* Truncated response      */
#define DRD       0x0100     /* Recursion desired       */
#define DRA       0x0080     /* Recursion available     */
#define DRCODE    0x000F     /* Response code mask, see below */

/*
 * Opcode possible values:
 */
#define DOPQUERY  0          /* a standard query */
#define DOPIQ     1          /* an inverse query */
#define DOPCQM    2          /* a completion query, multiple reply */
#define DOPCQU    3          /* a completion query, single reply   */
#define DUPDATE   5          /* DNS update (RFC-2136) */
/* the rest reserved for future */

/*
 * DNS server response codes; RCODE (stored in dom_errno)
 */
enum DNS_serv_resp {
     DNS_SRV_OK = 0,         /* 0 = okay response */
     DNS_SRV_FORM,           /* 1 = format error  */
     DNS_SRV_FAIL,           /* 2 = server failed */
     DNS_SRV_NAME,           /* 3 = name error (NXDOMAIN) */
     DNS_SRV_NOTIMPL,        /* 4 = function not implemented */
     DNS_SRV_REFUSE,         /* 5 = service refused */
     DNS_SRV_MAX = 15
   };

/*
 * DNS client codes (stored in dom_errno)
 */
enum DNS_client_code {
     DNS_CLI_SYSTEM = DNS_SRV_MAX,  /* See 'errno' */
     DNS_CLI_REFUSE,                /* Name server refused */
     DNS_CLI_USERQUIT,              /* User terminated via hook or ^C */
     DNS_CLI_NOSERV,                /* No nameserver defined */
     DNS_CLI_TIMEOUT,               /* Timeout, no reply */
     DNS_CLI_ILL_RESP,              /* Illegal/short response */
     DNS_CLI_ILL_IDNA,              /* Convert to/from international name failed */
     DNS_CLI_TOOBIG,                /* Name/label too large */
     DNS_CLI_NOIP,                  /* my_ip_addr == 0 */
     DNS_CLI_NOIPV6,                /* IPv6 DNS disabled */
     DNS_CLI_OTHER,                 /* Other general error */
     DNS_CLI_MAX
  };

/*
 * Query types (QTYPE)
 */
#define DTYPE_A     1         /* host address resource record (A) */
#define DTYPE_CNAME 5         /* host's canonical name record (CNAME) */
#define DTYPE_AAAA  28        /* host address resource record (AAAA) */
#define DTYPE_PTR   12        /* domain name ptr (PTR) */
#define DTYPE_SOA   6         /* start of authority zone (SOA) */
#define DTYPE_SRV   33        /* server selection (SRV), RFC-2052 */

/*
 * Query class (QCLASS)
 */
#define DIN         1         /* ARPA internet class */
#define DWILD       255       /* Wildcard for several classifications */

/*
 * DNS Query request/response header.
 */
struct DNS_query {
       struct DNS_head head;
       BYTE   body [DOMSIZE];
     };

/*
 * A resource record is made up of a compressed domain name followed by
 * this structure.  All of these words need to be byteswapped before use.
 * Part of 'struct DNS_query::body'.
 */
struct DNS_resource {
       WORD   rtype;              /* resource record type = DTYPE_A/AAAA */
       WORD   rclass;             /* RR class = DIN                      */
       DWORD  ttl;                /* time-to-live, changed to 32 bits    */
       WORD   rdlength;           /* length of next field                */
       BYTE   rdata [DOMSIZE-10]; /* data field                          */
     };

#define RESOURCE_HEAD_SIZE  10

#include <sys/packoff.h>

#define defaultdomain    NAMESPACE (defaultdomain)
#define def_domain       NAMESPACE (def_domain)
#define def_nameservers  NAMESPACE (def_nameservers)
#define dns_timeout      NAMESPACE (dns_timeout)
#define dns_recurse      NAMESPACE (dns_recurse)
#define dns_do_idna      NAMESPACE (dns_do_idna)
#define dns_do_ipv6      NAMESPACE (dns_do_ipv6)
#define dns_windns       NAMESPACE (dns_windns)
#define last_nameserver  NAMESPACE (last_nameserver)
#define dom_ttl          NAMESPACE (dom_ttl)
#define dom_errno        NAMESPACE (dom_errno)
#define dom_cname        NAMESPACE (dom_cname)
#define dom_a4list       NAMESPACE (dom_a4list)
#define dom_a6list       NAMESPACE (dom_a6list)

W32_DATA int (*_resolve_hook)(void);
W32_DATA BOOL  _resolve_exit;
W32_DATA BOOL  _resolve_timeout;

W32_DATA char  defaultdomain [MAX_HOSTLEN+1];
W32_DATA char *def_domain;
W32_DATA DWORD def_nameservers [MAX_NAMESERVERS];
W32_DATA WORD  last_nameserver;
W32_DATA int   dom_errno;

extern char  dom_cname [MAX_HOSTLEN+1];
extern DWORD dom_ttl;
extern BOOL  dns_do_ipv6;
extern BOOL  dns_do_idna;
extern WORD  dns_windns;
extern BOOL  called_from_resolve;
extern BOOL  called_from_ghbn;

extern DWORD       dom_a4list [MAX_ADDRESSES+1];
extern ip6_address dom_a6list [MAX_ADDRESSES+1];

W32_DATA UINT dns_timeout;
W32_DATA BOOL dns_recurse;

W32_FUNC DWORD       resolve      (const char *name);
W32_FUNC int         resolve_ip6  (const char *name, void *addr);
W32_FUNC const char *dom_strerror (int err);

/* In udp_rev.c
 */
extern int reverse_lookup_myip (void);
extern int reverse_resolve_ip4 (DWORD ipv4, char *result, size_t size);
extern int reverse_resolve_ip6 (const void *ipv6, char *result, size_t size);

/* In lookup.c
 */
W32_FUNC DWORD lookup_host (const char *host, char *ip_str);

#endif
