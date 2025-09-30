/*!\file sock_ini.h
 */
#ifndef _w32_SOCK_INI_H
#define _w32_SOCK_INI_H

enum eth_init_result {    /* pass to sock_init_err() */
     WERR_NO_ERROR,
     WERR_ILL_DOSX,       /* Watcom/HighC: illegal DOS-extender */
     WERR_NO_MEM,         /* All: No memory for misc. buffers */
     WERR_NO_DRIVER,      /* All: No network driver found (PKTDRVR/WinPcap) */
     WERR_PKT_ERROR,      /* All: General error in PKTDRVR/WinPcap interface */
     WERR_BOOTP_FAIL,     /* All: BOOTP protocol failed */
     WERR_DHCP_FAIL,      /* All: DHCP protocol failed */
     WERR_RARP_FAIL,      /* All: RARP protocol failed */
     WERR_NO_IPADDR,      /* All: Failed to find an IP-address */
     WERR_PPPOE_DISC      /* All: PPPoE discovery failed (timeout) */
   };

#define _bootp_on      NAMESPACE (_bootp_on)
#define _dhcp_on       NAMESPACE (_dhcp_on)
#define _dhcp6_on      NAMESPACE (_dhcp6_on)
#define _rarp_on       NAMESPACE (_rarp_on)
#define _do_mask_req   NAMESPACE (_do_mask_req)

#define survive_eth    NAMESPACE (survive_eth)
#define survive_bootp  NAMESPACE (survive_bootp)
#define survive_dhcp   NAMESPACE (survive_dhcp)
#define survive_rarp   NAMESPACE (survive_rarp)

W32_DATA int  _bootp_on;    /* boot-up through BOOTP and/or DHCP */
W32_DATA int  _dhcp_on;
W32_DATA int  _dhcp6_on;
W32_DATA int  _rarp_on;
W32_DATA BOOL _do_mask_req;
W32_DATA BOOL _watt_do_exit;
W32_DATA BOOL _watt_is_init;
W32_DATA BOOL _watt_no_config;

W32_DATA BOOL survive_eth, survive_bootp, survive_dhcp, survive_rarp;

W32_FUNC int           watt_sock_init (size_t tcp_sz, size_t udp_sz);
W32_FUNC void MS_CDECL sock_exit      (void);
W32_FUNC const char   *sock_init_err  (int rc);
W32_FUNC void          sock_sig_exit  (const char *msg, int sig) ATTR_NORETURN();

#if !defined(sock_init) && defined(TEST_PROG)
#define sock_init()  watt_sock_init (0, 0)
#endif

#endif
