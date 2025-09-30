/*!\file pcarp.h
 * ARP handler module.
 */
#ifndef _w32_PCARP_H
#define _w32_PCARP_H


/*!\struct gate_entry
 * Gateway table.
 */
struct gate_entry {
       DWORD  gate_ip;       /**< The IP address of gateway */
       DWORD  subnet;        /**< The subnet mask of gateway */
       DWORD  mask;
       BOOL   is_dead;       /* Dead Gate detection: */
       BOOL   echo_pending;  /*   echo-rply pending */
       DWORD  chk_timer;     /*   check interval timer */
     };

/*!\struct arp_entry
 * ARP cache table.
 */
struct arp_entry {
       DWORD       ip;            /**< IP address of ARP entry */
       eth_address hardware;      /**< MAC address of ARP entry */
       DWORD       expiry;        /**< 'pending timeout' or 'dynamic expiry' */
       DWORD       retransmit_to; /**< used only for re-requesting MAC while 'pending' */
       WORD        flags;         /**< ARP flags, see below */
     };

/** arp_entry::flags definitions.
 */
#define ARP_INUSE   0x01          /**< Entry is in use */
#define ARP_FIXED   0x02          /**< Entry is fixed (never times out) */
#define ARP_DYNAMIC 0x04          /**< Entry is dynamic (will timeout) */
#define ARP_PENDING 0x08          /**< Entry is pending a lookup */

/** Gateway functions.
 */
W32_FUNC int  _arp_list_gateways (struct gate_entry *gw, int max);
extern   BOOL _arp_add_gateway  (const char* config_string, DWORD ip);
extern   BOOL _arp_check_gateways (void);
extern   BOOL _arp_have_default_gw (void);
extern   void _arp_kill_gateways (void); /* GvB, replaces global variable */

/** Blocking ARP resolve functions.
 *
 * Doesn't return until success or time-out.
 */
W32_FUNC BOOL _arp_resolve      (DWORD ina, eth_address *eth);
W32_FUNC BOOL _arp_check_own_ip (eth_address *eth);

/** New non-blocking functions, GvB 2002-09.
 */
extern BOOL  arp_start_lookup   (DWORD ip);
extern BOOL  arp_lookup         (DWORD ip, eth_address *eth);
extern BOOL  arp_lookup_fixed   (DWORD ip, eth_address *eth);
extern BOOL  arp_lookup_pending (DWORD ip);

/** ARP cache functions.
 */
extern   BOOL _arp_add_cache (DWORD ip, const void *eth, BOOL expire);
extern   BOOL _arp_delete_cache (DWORD ip);
W32_FUNC int  _arp_list_cache (struct arp_entry *arp, int max);

/** 'Internal' interface to pctcp.c & others.
 */
extern void _arp_init (void);
extern BOOL _arp_reply  (const void *mac_dst, DWORD src_ip, DWORD dst_ip);
extern BOOL _arp_handler (const arp_Header *arp, BOOL brdcast);

/** ICMP redirection.
 */
extern BOOL _arp_register (DWORD use_this_gateway_ip, DWORD for_this_host_ip);

#if defined(USE_DEBUG)
  W32_FUNC void _arp_debug_dump (void);
#endif

#if defined(USE_SECURE_ARP)
  #include "pcsarp.h"

  W32_DATA BOOL (*_sarp_recv_hook) (const struct sarp_Packet *sarp);
  W32_DATA BOOL (*_sarp_xmit_hook) (struct sarp_Packet *sarp);
#endif

#endif
