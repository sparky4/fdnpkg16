/*!\file get_xby.h
 */
#ifndef _w32_GET_X_BY_Y_H
#define _w32_GET_X_BY_Y_H

#ifndef _REENTRANT
#define _REENTRANT   /* pull in the *_r() prototypes */
#endif

#ifndef __NETDB_H
#include <netdb.h>
#endif

#define MAX_SERV_ALIASES    5
#define MAX_HOST_ALIASES    5
#define MAX_NETENT_ALIASES  5
#define MAX_PROTO_ALIASES   0
#define MAX_CACHE_LIFE      (15*60)

#define ReadHostsFile     NAMESPACE (ReadHostsFile)
#define GetHostsFile      NAMESPACE (GetHostsFile)
#define CloseHostFile     NAMESPACE (CloseHostFile)

#define ReadHosts6File    NAMESPACE (ReadHosts6File)
#define CloseHost6File    NAMESPACE (CloseHost6File)
#define GetHosts6File     NAMESPACE (GetHosts6File)

#define ReadServFile      NAMESPACE (ReadServFile)
#define GetServFile       NAMESPACE (GetServFile)
#define CloseServFile     NAMESPACE (CloseServFile)

#define ReadProtoFile     NAMESPACE (ReadProtoFile)
#define GetProtoFile      NAMESPACE (GetProtoFile)
#define CloseProtoFile    NAMESPACE (CloseProtoFile)

#define ReadNetworksFile  NAMESPACE (ReadNetworksFile)
#define CloseNetworksFile NAMESPACE (CloseNetworksFile)
#define GetNetFile        NAMESPACE (GetNetFile)

#define InitEthersFile    NAMESPACE (InitEthersFile)
#define ReadEthersFile    NAMESPACE (ReadEthersFile)
#define GetEthersFile     NAMESPACE (GetEthersFile)
#define NumEtherEntries   NAMESPACE (NumEtherEntries)

#define gethostbyname6    NAMESPACE (gethostbyname6)
#define gethostbyaddr6    NAMESPACE (gethostbyaddr6)
#define gethostent6       NAMESPACE (gethostent6)

#define DumpHostsCache    NAMESPACE (DumpHostsCache)
#define DumpHosts6Cache   NAMESPACE (DumpHosts6Cache)


/*!\struct _hostent
 * Internal hostent structure.
 */
struct _hostent {
  char            *h_name;                         /* official name of host */
  char            *h_aliases[MAX_HOST_ALIASES+1];  /* hostname alias list */
  int              h_num_addr;                     /* how many real addr below */
  DWORD            h_address[MAX_ADDRESSES+1];     /* addresses (network order) */
  DWORD            h_real_ttl;                     /* TTL from udp_dom.c */
  time_t           h_timeout;                      /* cached entry expiry time */
  struct _hostent *h_next;                         /* next entry (or NULL) */
};

/*!\struct _hostent6
 * Internal hostent6 structure.
 */
struct _hostent6 {
  char             *h_name;                        /* official name of host */
  char             *h_aliases[MAX_HOST_ALIASES+1]; /* hostname alias list */
  int               h_num_addr;                    /* how many real addr below */
  ip6_address       h_address[MAX_ADDRESSES+1];    /* addresses */
  DWORD             h_real_ttl;                    /* TTL from udp_dom.c */
  time_t            h_timeout;                     /* cached entry expiry time */
  struct _hostent6 *h_next;
};

/*!\struct _netent
 * Internal netent structure.
 */
struct _netent {
  char           *n_name;                           /* official name of net */
  char           *n_aliases [MAX_NETENT_ALIASES+1]; /* alias list */
  int             n_addrtype;                       /* net address type */
  DWORD           n_net;                            /* network (host order) */
  struct _netent *n_next;
};

/*!\struct _protoent
 * Internal protoent structure.
 */
struct _protoent {
  char             *p_name;
  char             *p_aliases [MAX_PROTO_ALIASES+1];
  int               p_proto;
  struct _protoent *p_next;
};

/*!\struct _servent
 * Internal servent structure.
 */
struct _servent {
  char            *s_name;
  char            *s_aliases [MAX_SERV_ALIASES+1];
  int              s_port;
  char            *s_proto;
  struct _servent *s_next;
};

extern BOOL called_from_getai;

W32_DATA unsigned netdbCacheLife;

W32_FUNC void  ReadHostsFile    (const char *fname);  /* gethost.c */
W32_FUNC void  ReadServFile     (const char *fname);  /* getserv.c */
W32_FUNC void  ReadProtoFile    (const char *fname);  /* getprot.c */
W32_FUNC void  ReadNetworksFile (const char *fname);  /* getnet.c */
W32_FUNC void  ReadEthersFile   (void);               /* geteth.c */
W32_FUNC int   NumEtherEntries  (void);               /* geteth.c */
W32_FUNC void  InitEthersFile   (const char *fname);  /* geteth.c */

W32_FUNC void  CloseHostFile    (void);
W32_FUNC void  CloseServFile    (void);
W32_FUNC void  CloseProtoFile   (void);
W32_FUNC void  CloseNetworksFile(void);

W32_FUNC const char *GetHostsFile (void);
W32_FUNC const char *GetServFile  (void);
W32_FUNC const char *GetProtoFile (void);
W32_FUNC const char *GetNetFile   (void);
W32_FUNC const char *GetEthersFile(void);

W32_FUNC void DumpHostsCache (void);

#if defined(USE_BSD_API) && defined(USE_IPV6)  /* gethost6.c */
  W32_FUNC void            ReadHosts6File (const char *fname);
  W32_FUNC void            CloseHost6File (void);
  W32_FUNC void            DumpHosts6Cache (void);
  W32_FUNC const char     *GetHosts6File (void);

  W32_FUNC void            sethostent6 (int stayopen);
  W32_FUNC void            endhostent6 (void);
  W32_FUNC struct hostent *gethostent6 (void);
  W32_FUNC struct hostent *gethostbyname6 (const char *name);
  W32_FUNC struct hostent *gethostbyaddr6 (const void *addr);
#endif

#endif
