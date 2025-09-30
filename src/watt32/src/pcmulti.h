/*!\file pcmulti.h
 */
#ifndef _w32_PCMULTI_H
#define _w32_PCMULTI_H

extern int _multicast_on, _multicast_intvl;

#if defined(USE_MULTICAST)

  /**
   * Stuff for Multicast Support - JRM 6/7/93.
   */
  #define IPMULTI_SIZE    20            /**< the size of the ipmulti table     */
  #define MCAST_ALL_SYST  0xE0000001UL  /**< the default mcast addr 224.0.0.1  */

  /**\struct MultiCast
   *
   * Multicast internal structure.
   */
  struct MultiCast {
         DWORD       ip;           /**< IP address of group */
         eth_address ethaddr;      /**< Ethernet address of group */
         BYTE        processes;    /**< number of interested processes */
         DWORD       reply_timer;  /**< IGMP query reply timer */
         BOOL        active;       /**< is this an active entry */
       };

  extern int  join_mcast_group     (DWORD ip);
  extern int  leave_mcast_group    (DWORD ip);
  extern int  multi_to_eth         (DWORD ip, eth_address *eth);
  extern int  num_multicast_active (void);

  extern void igmp_handler (const in_Header *ip, BOOL brdcast);
#endif
#endif
