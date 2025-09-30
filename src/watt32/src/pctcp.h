/*!\file pctcp.h
 */
#ifndef _w32_PCTCP_H
#define _w32_PCTCP_H

/**
 * \def Timer definitions.
 * All timers are in milli-seconds.
 */
#define tcp_TIMEOUT       13000UL            /* timeout during a connection */
#define tcp_LONGTIMEOUT  (1000UL*sock_delay) /* timeout for open */
#define tcp_LASTACK_TIME  10000     /* timeout in the LASTACK state added AGW 5th Jan 2001 */

#define DEF_OPEN_TO       1000UL    /* # of msec in tcp-open (<=3s; RFC1122) */
#define DEF_CLOSE_TO      1000UL    /* # of msec for CLOSEWT state */
#define DEF_RTO_ADD       100UL     /* # of msec delay in RTT increment */
#define DEF_RTO_BASE      10UL      /* # of msec in RTO recalculation */
#define DEF_RTO_SCALE     64        /* RTO scale factor in _tcp_sendsoon() */
#define DEF_RST_TIME      100UL     /* # of msec before sending RST */
#define DEF_RETRAN_TIME   10UL      /* do retransmit logic every 10ms */
#define DEF_RECV_WIN     (16*1024)  /* default receive window, 16kB */
#define MAX_DAEMONS       20        /* max # of background daemons */
#define DAEMON_PERIOD     500       /* run daemons every 500msec */


/*
 * S. Lawson - define a short TIME_WAIT timeout. It should be from
 * .5 to 4 minutes (2MSL) but it's not really practical for us.
 * 2 secs will hopefully handle the case where ACK must be retransmitted,
 * but can't protect future connections on the same port from old packets.
 */
#define tcp_TIMEWT_TO 2000UL

#ifndef __NETINET_TCP_SEQ_H

  /**
   * \def SEQ_* macros.
   *
   * TCP sequence numbers are 32 bit integers operated
   * on with modular arithmetic.  These macros can be
   * used to compare such integers.
   */
  #define SEQ_LT(a,b)            ((long)((a) - (b)) < 0)
  #define SEQ_LEQ(a,b)           ((long)((a) - (b)) <= 0)
  #define SEQ_GT(a,b)            ((long)((a) - (b)) > 0)
  #define SEQ_GEQ(a,b)           ((long)((a) - (b)) >= 0)
  #define SEQ_BETWEEN(seq,lo,hi) ((seq - lo) <= (hi - lo))
                              /* (SEG_GEQ(seq,lo) && SEG_LEQ(seq,hi)) */
#endif

/**
 * \def INIT_SEQ
 * We use 32-bit from system-timer as initial sequence number
 * (ISN, network order). Maybe not the best choice (easy guessable).
 * The ISN should wrap only once a day.
 */
#define INIT_SEQ()  intel (Random(1,0xFFFFFFFF)) /* intel (set_timeout(1)) */

/**
 * Van Jacobson's Algorithm; max std. average and std. deviation
 */
#define DEF_MAX_VJSA    60000U
#define DEF_MAX_VJSD    20000U
#define INIT_VJSA       220

/**
 * The TCP options.
 */
#define TCPOPT_EOL        0       /**< end-of-option list */
#define TCPOPT_NOP        1       /**< no-operation */
#define TCPOPT_MAXSEG     2       /**< maximum segment size */
#define TCPOPT_WINDOW     3       /**< window scale factor        RFC1072 */
#define TCPOPT_SACK_PERM  4       /**< selective ack permitted    RFC1072 */
#define TCPOPT_SACK       5       /**< selective ack              RFC1072 */
#define TCPOPT_ECHO       6       /**< echo-request               RFC1072 */
#define TCPOPT_ECHOREPLY  7       /**< echo-reply                 RFC1072 */
#define TCPOPT_TIMESTAMP  8       /**< timestamps                 RFC1323 */
#define TCPOPT_CC         11      /**< T/TCP CC options           RFC1644 */
#define TCPOPT_CCNEW      12      /**< T/TCP CC options           RFC1644 */
#define TCPOPT_CCECHO     13      /**< T/TCP CC options           RFC1644 */
#define TCPOPT_CHKSUM_REQ 14      /**< Alternate checksum request RFC1146 */
#define TCPOPT_CHKSUM_DAT 15      /**< Alternate checksum data    RFC1146 */
#define TCPOPT_SIGNATURE  19      /**< MD5 signature              RFC2385 */
#define   TCPOPT_SIGN_LEN 16

#define TCP_MAX_WINSHIFT  14      /**< maximum window shift */


/**
 * MTU defaults to 1500 (ETH_MAX_DATA).
 * TCP_OVERHEAD == 40.
 */
#define MSS_MAX     (_mtu - TCP_OVERHEAD)
#define MSS_MIN     (576 - TCP_OVERHEAD)
#define MSS_REDUCE  20   /* do better than this (exponentially decrease) */

#define my_ip_addr      NAMESPACE (my_ip_addr)
#define sin_mask        NAMESPACE (sin_mask)
#define block_tcp       NAMESPACE (block_tcp)
#define block_udp       NAMESPACE (block_udp)
#define block_ip        NAMESPACE (block_ip)
#define block_icmp      NAMESPACE (block_icmp)
#define use_rand_lport  NAMESPACE (use_rand_lport)

#define hostname        NAMESPACE (hostname)
#define _mtu            NAMESPACE (_mtu)
#define _mss            NAMESPACE (_mss)
#define mtu_discover    NAMESPACE (mtu_discover)
#define mtu_blackhole   NAMESPACE (mtu_blackhole)
#define tcp_nagle       NAMESPACE (tcp_nagle)
#define tcp_keep_idle   NAMESPACE (tcp_keep_idle)
#define tcp_keep_intvl  NAMESPACE (tcp_keep_intvl)
#define tcp_max_idle    NAMESPACE (tcp_max_idle)
#define tcp_opt_ts      NAMESPACE (tcp_opt_ts)
#define tcp_opt_sack    NAMESPACE (tcp_opt_sack)
#define tcp_opt_wscale  NAMESPACE (tcp_opt_wscale)
#define tcp_recv_win    NAMESPACE (tcp_recv_win)

W32_DATA unsigned _mtu, _mss;
W32_DATA DWORD    my_ip_addr;
W32_DATA DWORD    sin_mask;
W32_DATA BOOL     block_tcp;
W32_DATA BOOL     block_udp;
W32_DATA BOOL     block_icmp;
W32_DATA BOOL     block_ip;
W32_DATA unsigned tcp_keep_idle, tcp_keep_intvl, tcp_max_idle;
W32_DATA char     hostname [MAX_HOSTLEN+1];

extern BOOL mtu_discover;
extern BOOL mtu_blackhole;
extern BOOL use_rand_lport;
extern BOOL tcp_nagle;
extern BOOL tcp_opt_ts;
extern BOOL tcp_opt_sack;
extern BOOL tcp_opt_wscale;

extern _tcp_Socket *_tcp_allsocs;
extern _udp_Socket *_udp_allsocs;

/* Same as in <tcp.h>
 */
#define sock_wait_established(s,seconds,fn,statusptr) \
        do {                                          \
           if (_ip_delay0 (s,seconds,fn,statusptr))   \
              goto sock_err;                          \
        } while (0)

#define sock_wait_input(s,seconds,fn,statusptr)       \
        do {                                          \
           if (_ip_delay1 (s,seconds,fn,statusptr))   \
              goto sock_err;                          \
        } while (0)

#define sock_wait_closed(s,seconds,fn,statusptr)      \
        do {                                          \
           if (_ip_delay2(s,seconds,fn,statusptr))    \
              goto sock_err;                          \
        } while (0)

#define sock_tick(s, statusptr)                       \
        do {                                          \
           if (!tcp_tick(s)) {                        \
              if (statusptr) *statusptr = 1;          \
              goto sock_err;                          \
           }                                          \
        } while (0)


extern void _udp_cancel (const in_Header*, int, int, const char *, const void *);
extern void _tcp_cancel (const in_Header*, int, int, const char *, const void *);

extern void _tcp_close    (_tcp_Socket *s);
extern void  tcp_rtt_add  (const _tcp_Socket *s, UINT rto, UINT MTU);
extern void  tcp_rtt_clr  (const _tcp_Socket *s);
extern BOOL  tcp_rtt_get  (const _tcp_Socket *s, UINT *rto, UINT *MTU);

extern int   tcp_established (const _tcp_Socket *s);
extern int  _tcp_send        (_tcp_Socket *s, char *file, unsigned line);
extern int  _tcp_sendsoon    (_tcp_Socket *s, char *file, unsigned line);
extern int  _tcp_keepalive   (_tcp_Socket *s);

extern void tcp_Retransmitter (BOOL force);

extern _udp_Socket *_udp_handler  (const in_Header *ip, BOOL broadcast);
extern _tcp_Socket *_tcp_handler  (const in_Header *ip, BOOL broadcast);
extern _tcp_Socket *_tcp_unthread (_tcp_Socket *s, BOOL free_tx);
extern _tcp_Socket *_tcp_abort    (_tcp_Socket *s, const char *file, unsigned line);
extern int          _tcp_send_reset (_tcp_Socket *s, const in_Header *ip,
                                     const tcp_Header *tcp, const char *file,
                                     unsigned line);

#define TCP_SEND(s)     _tcp_send     (s, __FILE__, __LINE__)
#define TCP_SENDSOON(s) _tcp_sendsoon (s, __FILE__, __LINE__)
#define TCP_ABORT(s)    _tcp_abort    (s, __FILE__, __LINE__)

#define TCP_SEND_RESET(s, ip, tcp) \
       _tcp_send_reset(s, ip, tcp, __FILE__, __LINE__)

/*
 * Public API
 */
W32_DATA unsigned tcp_OPEN_TO;  /* these are really not publics (but tcpinfo needs them) */
W32_DATA unsigned tcp_CLOSE_TO;
W32_DATA unsigned tcp_RTO_ADD;
W32_DATA unsigned tcp_RTO_BASE;
W32_DATA unsigned tcp_RTO_SCALE;
W32_DATA unsigned tcp_RST_TIME;
W32_DATA unsigned tcp_RETRAN_TIME;
W32_DATA unsigned tcp_MAX_VJSA;
W32_DATA unsigned tcp_MAX_VJSD;
W32_DATA DWORD    tcp_recv_win;

W32_FUNC WORD tcp_tick   (sock_type *s);
W32_FUNC int  tcp_open   (_tcp_Socket *s, WORD lport, DWORD ina, WORD port, ProtoHandler handler);
W32_FUNC int  tcp_listen (_tcp_Socket *s, WORD lport, DWORD ina, WORD port, ProtoHandler handler, WORD timeout);
W32_FUNC int  udp_open   (_udp_Socket *s, WORD lport, DWORD ina, WORD port, ProtoHandler handler);
W32_FUNC int  udp_listen (_udp_Socket *s, WORD lport, DWORD ina, WORD port, ProtoHandler handler);
W32_FUNC void udp_SetTTL (_udp_Socket *s, BYTE ttl);

W32_FUNC void   sock_abort     (sock_type *s);
W32_FUNC int    sock_read      (sock_type *s, BYTE *dp, int len);
W32_FUNC int    sock_fastread  (sock_type *s, BYTE *dp, int len);
W32_FUNC int    sock_write     (sock_type *s, const BYTE *dp, int len);
W32_FUNC int    sock_fastwrite (sock_type *s, const BYTE *dp, int len);
W32_FUNC int    sock_enqueue   (sock_type *s, const BYTE *dp, int len);
W32_FUNC void   sock_noflush   (sock_type *s);
W32_FUNC void   sock_flush     (sock_type *s);
W32_FUNC void   sock_flushnext (sock_type *s);
W32_FUNC BYTE   sock_putc      (sock_type *s, BYTE c);
W32_FUNC int    sock_getc      (sock_type *s);
W32_FUNC int    sock_puts      (sock_type *s, const BYTE *dp);
W32_FUNC int    sock_gets      (sock_type *s, BYTE *dp, int n);
W32_FUNC WORD   sock_dataready (sock_type *s);
W32_FUNC int    sock_close     (sock_type *s);
W32_FUNC void (*sock_yield     (_tcp_Socket *s, void (*fn)(void))) (void);
W32_FUNC WORD   sock_mode      (sock_type *s, WORD mode);
W32_FUNC int    sock_sselect   (const sock_type *s, int state);
W32_FUNC int    sock_keepalive (sock_type *s);
W32_FUNC int    addwattcpd     (void (*p)(void));
W32_FUNC int    delwattcpd     (void (*p)(void));

W32_FUNC int MS_CDECL sock_printf (sock_type *s, const char *fmt, ...)
                      ATTR_PRINTF (2,3);

W32_FUNC int MS_CDECL sock_scanf (sock_type *s, const char *fmt, ...)
                      ATTR_SCANF (2,3);

#define SET_ERR_MSG(s, msg) \
        do { \
          if (!s->err_msg && msg) \
             s->err_msg = StrLcpy (s->err_buf, msg, sizeof(s->err_buf)); \
        } while (0)

/*!\struct tcp_rtt
 *
 * A simple RTT cache based on Phil Karn's KA9Q.
 */
struct tcp_rtt {
       DWORD  ip;     /**< IP-address of this entry */
       UINT   rto;    /**< Round-trip timeout for this entry */
       UINT   MTU;    /**< Path-MTU discovered for this entry */
     };

#define RTTCACHE  16  /**< # of TCP round-trip-time cache entries */


#if defined(USE_BSD_API)
  /*
   * Operations for the '_bsd_socket_hook'.
   */
  enum BSD_SOCKET_OPS {
       BSO_FIND_SOCK = 1,   /* return a 'Socket*' from '_tcp_Socket*' */
       BSO_SYN_CALLBACK,    /* called on SYN received */
       BSO_RST_CALLBACK,    /* called on RST received */
       BSO_IP4_RAW,         /* called on IPv4 input. */
       BSO_IP6_RAW,         /* called on IPv6 input. */
       BSO_DEBUG,           /* called to perform SO_DEBUG stuff */
     };

  extern void * (MS_CDECL *_bsd_socket_hook) (enum BSD_SOCKET_OPS op, ...);
#endif


/* In ports.c
 */
extern WORD  init_localport  (void);
extern WORD  find_free_port  (WORD oldport, BOOL sleep_msl);
extern int   grab_localport  (WORD port);
extern int   reuse_localport (WORD port);
extern int   maybe_reuse_localport (_tcp_Socket *s);

/* In sock_in.c
 */
W32_FUNC void  ip_timer_init    (sock_type *s, unsigned);
W32_FUNC int   ip_timer_expired (const sock_type *s);
W32_FUNC int  _ip_delay0        (sock_type *, int, UserHandler, int *);
W32_FUNC int  _ip_delay1        (sock_type *, int, UserHandler, int *);
W32_FUNC int  _ip_delay2        (sock_type *, int, UserHandler, int *);
W32_FUNC int   sock_timeout     (sock_type *, int);
W32_FUNC int   sock_established (sock_type *);

#endif
