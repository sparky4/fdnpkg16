/*!\file winpcap.h
 *
 * Only included on Win32
 */
#ifndef _w32_WINPCAP_H
#define _w32_WINPCAP_H

#if !defined(_w32_PCPKT_H)
#error Include this file inside or after pcpkt.h only.
#endif

/**
 * Make WinPcap/npf.sys behave like a Packet-driver.
 * Errors below 0 are WinPcap.c errors. Others come from GetLastError().
 * \todo Add some npf/NDIS specific errors?
 */
#define PDERR_GEN_FAIL      -1
#define PDERR_NO_DRIVER     -2
#define PDERR_NO_CLASS      -3
#define PDERR_NO_MULTICAST  -4
#define PDERR_NO_SPACE      -5

/** Driver classes. Use same values as in <NtDDNdis.h>.
 */
#define PDCLASS_ETHER     0       /**< NdisMedium802_3 */
#define PDCLASS_TOKEN     1       /**< NdisMedium802_5 */
#define PDCLASS_FDDI      3       /**< NdisMediumFddi */
#define PDCLASS_ARCNET    6       /**< NdisMediumArcnetRaw ? */
#define PDCLASS_SLIP      0xFF01  /**< Serial Line IP (unsupported) */
#define PDCLASS_AX25      0xFF02  /**< Amateur X.25 (unsupported) */
#define PDCLASS_TOKEN_RIF 0xFF03  /**< IEEE 802.5 w/expanded RIFs (unsupported) */
#define PDCLASS_PPP       0xFF04  /**< PPP/Wan (unsupported) */
#define PDCLASS_UNKNOWN   0xFFFF

enum ReceiveModes {
     RXMODE_OFF        = 0x00,    /**< possible to turn off? */
     RXMODE_DIRECT     = 0x01,    /**< NDIS_PACKET_TYPE_DIRECTED */
     RXMODE_MULTICAST1 = 0x02,    /**< NDIS_PACKET_TYPE_MULTICAST */
     RXMODE_MULTICAST2 = 0x04,    /**< NDIS_PACKET_TYPE_ALL_MULTICAST */
     RXMODE_BROADCAST  = 0x08,    /**< NDIS_PACKET_TYPE_BROADCAST */
     RXMODE_PROMISCOUS = 0x20,    /**< NDIS_PACKET_TYPE_PROMISCUOUS */
     RXMODE_ALL_LOCAL  = 0x80     /**< NDIS_PACKET_TYPE_ALL_LOCAL (direct+bc+mc1) */
     /* we have no use for the other NDIS_PACKET_TYPE defs */
   };

/* NPF defaults to mode 0 after openening adapter.
 */
#define RXMODE_DEFAULT  (RXMODE_DIRECT | RXMODE_BROADCAST)

W32_FUNC int  pkt_eth_init      (mac_address *eth);
W32_FUNC int  pkt_release       (void);
W32_FUNC int  pkt_reset_handle  (void *handle);
W32_FUNC int  pkt_send          (const void *tx, int length);
W32_FUNC int  pkt_buf_wipe      (void);
W32_FUNC void pkt_free_pkt      (const void *pkt);
W32_FUNC int  pkt_waiting       (void);
W32_FUNC int  pkt_set_addr      (const void *eth);
W32_FUNC int  pkt_get_addr      (mac_address *eth);
W32_FUNC int  pkt_get_mtu       (void);
W32_FUNC int  pkt_get_drvr_ver  (DWORD *ver);
W32_FUNC int  pkt_get_api_ver   (DWORD *ver);
W32_FUNC int  pkt_set_rcv_mode  (int mode);
W32_FUNC int  pkt_get_rcv_mode  (void);
W32_FUNC BOOL pkt_check_address (DWORD my_ip);

extern struct pkt_rx_element *pkt_poll_recv (void);

#if defined(USE_MULTICAST)
  extern int pkt_get_multicast_list (mac_address *listbuf, int *len);
  extern int pkt_set_multicast_list (const void *listbuf, int len);
#endif

W32_FUNC int pkt_get_stats (struct PktStats *stats, struct PktStats *total);

/**\struct pkt_info
 *
 * Placeholder for vital data accessed by capture thread.
 */
struct pkt_info {
       const void   *adapter;            /* opaque ADAPTER object */
       const void   *adapter_info;       /* opaque ADAPTER_INFO object */
       void         *npf_buf;            /* buffer for ReadFile() */
       int           npf_buf_size;       /* size of above buffer */
       HANDLE        recv_thread;        /* thread for capturing */
       WORD          pkt_ip_ofs;         /* store length of MAC-header */
       WORD          pkt_type_ofs;       /* offset to MAC-type */
       volatile BOOL stop_thread;        /* signal thread to stop */
       volatile BOOL thread_stopped;     /* did it stop gracefully? */
       const char   *error;              /* Last error (NULL if none) */
       struct pkt_ringbuf    pkt_queue;  /* Ring-struct for enqueueing */
       struct pkt_rx_element rx_buf [RX_BUFS]; /* the receive buffer */
     };

W32_DATA char _pktdrvr_descr[];

W32_DATA struct pkt_info *_pkt_inf;

#endif

