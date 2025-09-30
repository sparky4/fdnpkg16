/*!\file pcarp.c
 *
 * Address Resolution Protocol.
 *
 * 2002-09 Gundolf von Bachhaus:
 *   90% rewrite - Optional non-blocking ARP lookup & redirect handling
 *   Gateway / Route / ARP data stored & accessed sererately
 *   Module encapsulated - no global variables
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "copyrigh.h"
#include "wattcp.h"
#include "strings.h"
#include "language.h"
#include "netaddr.h"
#include "misc.h"
#include "timer.h"
#include "rs232.h"
#include "ip4_in.h"
#include "ip4_out.h"
#include "sock_ini.h"
#include "chksum.h"
#include "pcdbug.h"
#include "pctcp.h"
#include "pcsed.h"
#include "pcconfig.h"
#include "pcqueue.h"
#include "pcicmp.h"
#include "pcdhcp.h"
#include "pcpkt.h"
#include "pcarp.h"

#ifndef __inline
#define __inline
#endif

#if defined(USE_SECURE_ARP)
  /*
   * Allocated here so we don't link in OpenSSL libs by default.
   * Application must call sarp_init() before sock_init() to enable.
   */
  int (*_sarp_recv_hook) (const struct sarp_Packet*) = NULL;
  int (*_sarp_xmit_hook) (struct sarp_Packet*)       = NULL;
#endif

/* Local parameters.
 */
static int  arp_timeout    = 2;      /* 2 seconds ARP timeout    */
static int  arp_alive      = 300;    /* 5 min ARP cache lifespan */
static int  arp_rexmit_to  = 250;    /* 250 milliseconds per try */
static int  arp_num_cache  = 64;     /* # of entries in ARP cache (not yet) */
static BOOL dead_gw_detect = FALSE;  /* Enable Dead Gateway detection */
static BOOL arp_gratiotous = FALSE;

static BOOL LAN_lookup (DWORD ip, eth_address *eth);
static BOOL is_on_LAN  (DWORD ip);
static void arp_daemon (void);

/**
 * GATEWAY HANDLING.
 */
#define DEAD_GW_TIMEOUT    10000   /* 10 sec */
#define gate_top_of_cache  8

static int gate_top = 0;

static struct gate_entry gate_list [gate_top_of_cache];


#if defined(USE_DEBUG)
/**
 * Return list of currently defined gateways.
 *
 * \note Max 'num' gateways is returned in 'gw[]' buffer.
 */
int _arp_list_gateways (struct gate_entry *gw, int max)
{
  int i;

  for (i = 0; i < gate_top && i < max; i++, gw++)
  {
    memset (gw, 0, sizeof(*gw));
    gw->gate_ip = gate_list[i].gate_ip;
    gw->subnet  = gate_list[i].subnet;
    gw->mask    = gate_list[i].mask;
#if defined(USE_DEAD_GWD)
    gw->echo_pending = gate_list[i].echo_pending;
    gw->is_dead      = gate_list[i].is_dead;
#endif
  }
  return (i);
}
#endif

#if defined(USE_DEAD_GWD)
/**
 * Send a ping (with ttl=1) to a gateway.
 * Ref. RFC-1122.
 */
static WORD icmp_id  = 0;
static WORD icmp_seq = 0;

static int ping_gateway (DWORD host, void *eth)
{
  struct ping_pkt  *pkt;
  struct icmp_echo *icmp;
  struct in_Header *ip;
  int    len;

  pkt  = (struct ping_pkt*) _eth_formatpacket (eth, IP4_TYPE);
  ip   = &pkt->in;
  icmp = &pkt->icmp;
  len  = sizeof (*icmp);
  icmp_id = (WORD) set_timeout (0);  /* "random" id */

  icmp->type       = ICMP_ECHO;
  icmp->code       = 0;
  icmp->index      = 1;
  icmp->identifier = icmp_id;
  icmp->sequence   = icmp_seq++;
  icmp->checksum   = 0;
  icmp->checksum   = ~CHECKSUM (icmp, len);

  return IP4_OUTPUT (ip, 0, intel(host), ICMP_PROTO, 1,
                     (BYTE)_default_tos, 0, len, NULL);
}

/*
 * This functiom is called once each second.
 */
static void check_dead_gw (void)
{
  char   buf[20];
  static int i = 0;

  if (i >= gate_top)
      i = 0;

  for ( ; i < gate_top; i++)
  {
    struct gate_entry *gw = &gate_list [i];
    eth_address        eth;

    if (!is_on_LAN(gw->gate_ip))
       continue;

    if (!LAN_lookup(gw->gate_ip,&eth))
       continue;

    if (gw->chk_timer == 0UL || !chk_timeout(gw->chk_timer))
    {
      gw->chk_timer = set_timeout (DEAD_GW_TIMEOUT);
      continue;
    }

    if (gw->echo_pending &&
        _chk_ping(gw->gate_ip,NULL) == (DWORD)-1)
    {
      gw->is_dead      = TRUE;
      gw->echo_pending = FALSE;
      TCP_CONSOLE_MSG (1, ("Dead default GW %s (%d) detected\n",
                       _inet_ntoa(NULL,gw->gate_ip), i));
    }

    if (ping_gateway (gw->gate_ip, eth))
       gw->echo_pending = TRUE;

    gw->chk_timer = set_timeout (DEAD_GW_TIMEOUT);
    return;  /* only one ping per interval */
  }
}
#endif


/**
 * Add a gateway to the routing table.
 *
 * The format of 'config_string' is:
 *   IP-address [, subnet] , mask]]
 *
 * If 'config_string' is NULL, simply add 'ip' with zero
 * mask and subnet.
 */
BOOL _arp_add_gateway (const char *config_string, DWORD ip)
{
  struct gate_entry *gw;
  DWORD  subnet = 0UL;
  DWORD  mask   = 0UL;
  int    i;

  SIO_TRACE (("_arp_add_gateway"));

  if (config_string)
  {
    const char* subnetp, *maskp;

    /* Get gateways IP address from string
     */
    ip = aton (config_string);
    if (ip == 0)
       return (FALSE); /* invalid */

    /* Check if optional subnet was supplied
     */
    subnetp = strchr (config_string, ',');
    if (subnetp)
    {
      /* NB: atoi (used in aton) stops conversion when the first non-number
       * is hit, so there is no need to manually null-terminate the string.
       * i.e. aton ("123.123.123.123,blabla") = 123.123.123.123
       */
      subnet = aton (++subnetp);

      /* Check if optional mask was supplied
       */
      maskp = strchr (subnetp, ',');
      if (maskp)
         mask = aton (++maskp);
      else /* No mask was supplied, we derive it from the supplied subnet */
      {
        switch (subnet >> 30)
        {
          case  0:
          case  1:
                mask = 0xFF000000UL;
                break;
          case  2:
                mask = 0xFFFFFE00UL;   /* minimal class B */
                break;
          case  3:
          default:
                mask = 0xFFFFFF00UL;
                break;
        }
      }
    }
  }
  else /* (config_string == NULL) */
  {
    if (ip == 0UL)
    {
      outsnl ("_arp_add_gateway(): Illegal router");
      return (FALSE); /* args invalid */
    }
  }

  /* Figure out where to put our new gateway
   */
  gw = NULL;

  /* Check if gateway is already is in list
   */
  for (i = 0; i < gate_top; i++)
      if (gate_list[i].gate_ip == ip)
      {
        gw = &gate_list[i];
        break;
      }

  /* If a 'new' gateway, we check if we have enough room and simply
   * add it to the end of the list.
   *
   * There is not much point in sorting the list, as the 'whole' list is
   * scanned when a gateway is needed. Usually there will only be 1 anyway.
   * If we do sort, we would use plain old insert-sort and NOT quicksort,
   * as quicksort goes O(n^2) on an already sorted list, plus it has a high
   * overhead that especially hurts on a tiny list like this.
   */
  if (gw == NULL)
  {
    if (gate_top+1 >= DIM(gate_list))
    {
      outsnl (_LANG("Gateway table full"));
      return (FALSE);  /* no more room */
    }
    gw = &gate_list [gate_top++];
  }

  /* Fill in new entry
   */
  memset (gw, 0, sizeof(*gw));
  gw->gate_ip = ip;
  gw->mask    = mask;
  gw->subnet  = subnet;
  return (TRUE);
}

/**
 * Delete all gateways.
 */
void _arp_kill_gateways (void)
{
  SIO_TRACE (("_arp_kill_gateways"));
  gate_top = 0;
}

/**
 * Check if we have at least one default gateway.
 */
BOOL _arp_have_default_gw (void)
{
  int i, num = 0;

  SIO_TRACE (("_arp_have_default_gw"));
  for (i = 0; i < gate_top; i++)
      if (gate_list[i].subnet == 0UL)
         num++;
  return (num > 0);
}


/**
 * ARP HANDLING.
 */

/* ARP cache table internal structure:
 * \verbatim
 *
 * Index pointers       ARP cache table
 * --------------       ---------------
 *
 * (top_of_cache) ->
 *  (= top_pending)   ----------------------
 *                    | ...
 *                    | PENDING ARP ENTRIES (requested/ing - no reply yet)
 * first_pending ->   | ... grow downwards (into free slots)
 *  (= top_free)      ----------------------
 *                    | ...
 *                    | ...
 *                    | FREE SLOTS
 *                    | ...
 * first_free ---->   | ...
 *  (= top_dynamic)   ----------------------
 *                    | ... grow upwards (into free slots)
 *                    | ...
 *                    | "DYNAMIC" ARP ENTRIES
 * first_dynamic ->   | ...
 *  (= top_fixed)     ----------------------
 *                    | ... grow upwards (rolling dynamic entries upwards)
 *                    | FIXED ARP ENTRIES
 *  (first_dynamic=0) | ...
 *                    ----------------------
 * \endverbatim
 *
 * \note
 *  - "Top" means last entry + 1.
 *  - The entries inside each section are not ordered in any way.
 *  - The ARP cache only holds entries of hosts that are on our LAN.
 *  - Connections to hosts outside of our LAN are done over a gateway, and
 *    the IP of the gateway used (per host) is stored in the "route cache"
 *    (below).
 */

#define arp_top_of_cache    64 /**< would need to be made a variable if ARP
                                *   table were auto-resizing
                                */
#define arp_top_pending     arp_top_of_cache
#define arp_top_free        arp_first_pending
#define arp_top_dynamic     arp_first_free
#define arp_top_fixed       arp_first_dynamic
#define arp_first_fixed     0

static int arp_first_pending = arp_top_pending;
static int arp_first_free    = 0;
static int arp_first_dynamic = 0;

static struct arp_entry arp_list [arp_top_of_cache];

/**
 * Low-level ARP send function.
 */
static __inline BOOL arp_send (const arp_Header *arp, unsigned line)
{
  WORD len = sizeof(*arp);

  SIO_TRACE (("_arp_send"));

#if defined(USE_SECURE_ARP)
  if (_sarp_xmit_hook)  /* put the auth header in */
     len = (*_sarp_xmit_hook) ((struct sarp_Packet*)arp);
#else
  ARGSUSED (arp);
#endif
  return _eth_send (len, NULL, __FILE__, line);
}


/**
 * Send broadcast ARP request.
 */
static BOOL arp_send_request (DWORD ip)
{
  arp_Header *arp = (arp_Header*) _eth_formatpacket (&_eth_brdcast[0], ARP_TYPE);

  SIO_TRACE (("_arp_send_request"));

  arp->hwType       = intel16 (_eth_get_hwtype(NULL,NULL));
  arp->protType     = IP4_TYPE;
  arp->hwAddrLen    = sizeof (eth_address);
  arp->protoAddrLen = sizeof (ip);
  arp->opcode       = ARP_REQUEST;
  arp->srcIPAddr    = intel (my_ip_addr);
  arp->dstIPAddr    = intel (ip);
  memcpy (arp->srcEthAddr, _eth_addr, sizeof(arp->srcEthAddr));
  memset (arp->dstEthAddr, 0, sizeof(arp->dstEthAddr));
  return arp_send (arp, __LINE__);
}


/**
 * Send unicast/broadcast ARP reply.
 * 'src_ip' and 'dst_ip' on network order.
 */
BOOL _arp_reply (const void *mac_dst, DWORD src_ip, DWORD dst_ip)
{
  arp_Header *arp;

  SIO_TRACE (("_arp_reply"));

  if (!mac_dst)
     mac_dst = &_eth_brdcast;

  arp = (arp_Header*) _eth_formatpacket (mac_dst, ARP_TYPE);
  arp->hwType       = intel16 (_eth_get_hwtype(NULL,NULL));
  arp->protType     = IP4_TYPE;
  arp->hwAddrLen    = sizeof (mac_address);
  arp->protoAddrLen = sizeof (dst_ip);
  arp->opcode       = ARP_REPLY;
  arp->dstIPAddr    = src_ip;
  arp->srcIPAddr    = dst_ip;

  memcpy (arp->srcEthAddr, _eth_addr,sizeof(mac_address));
  memcpy (arp->dstEthAddr, mac_dst,  sizeof(mac_address));
  return arp_send (arp, __LINE__);
}

/**
 * Move an ARP entry from \b from_index to \b to_index.
 */
static __inline void arp_move_entry (int to_index, int from_index)
{
  memcpy (&arp_list[to_index], &arp_list[from_index], sizeof(struct arp_entry));
}

/**
 * Start a host lookup on the LAN.
 *
 * This is called by 'higher' routines to start an
 * ARP request. This function is non-blocking, i.e. returns 'immediately'.
 */
static BOOL LAN_start_lookup (DWORD ip)
{
  struct arp_entry *ae;
  int    i;

  SIO_TRACE (("LAN_start_lookup"));

  /* Ignore if IP is already in any list section (pending, fixed, dynamic)
   */
  for (i = arp_first_pending; i < arp_top_pending; i++)
      if (arp_list[i].ip == ip)
          return (TRUE);

  for (i = arp_first_dynamic; i < arp_top_dynamic; i++)
      if (arp_list[i].ip == ip)
         return (TRUE);

  for (i = arp_first_fixed; i < arp_top_fixed; i++)
      if (arp_list[i].ip == ip)
         return (TRUE);

  /* Figure out where to put the new guy
   */
  if (arp_first_free < arp_top_free) /* Do we have any free slots? */
  {
    /* yes, ok! */
  }
  else if (arp_first_dynamic < arp_top_dynamic) /* any dynamic entries? */
  {
    /* This new request is probably more important than an existing
     * dynamic entry, so we sacrifice the top dynamic entry. It might be
     * neater to kill the oldest entry, but all this shouldn't happen anyway.
     * NB: Table size reallocation would go here.
     */
    --arp_top_dynamic; /* nuke top entry */
    outsnl (_LANG("ARP table overflow"));
  }
  else /* No more room - table is full with pending + fixed entries. */
  {
    outsnl (_LANG("ARP table full"));
    return (FALSE);       /* failed, nothing we can do right now. */
  }

  /* Fill new slot, send out ARP request
   */
  ae = &arp_list [--arp_first_pending];
  ae->ip     = ip;
  ae->expiry = set_timeout (1000UL * arp_timeout);
  ae->flags  = (ARP_INUSE | ARP_PENDING);

  /* If request fails, we try again a little sooner
   */
  if (!arp_send_request(ip))
       ae->retransmit_to = set_timeout (arp_rexmit_to / 4);
  else ae->retransmit_to = set_timeout (arp_rexmit_to);

  return (TRUE);      /* ok, new request logged */
}

/**
 * Lookup host in fixed/dynamic list.
 */
static BOOL LAN_lookup (DWORD ip, eth_address *eth)
{
  int i;

  SIO_TRACE (("LAN_lookup"));

  /* Check in dynamic + fixed list section
   */
  for (i = arp_first_fixed; i < arp_top_dynamic; i++)
  {
    if (arp_list[i].ip != ip)
       continue;
    if (eth)
       memcpy (eth, arp_list[i].hardware, sizeof(*eth));
    return (TRUE);
  }
  return (FALSE);
}

/**
 * Lookup host in pending list.
 */
static BOOL LAN_lookup_pending (DWORD ip)
{
  int i;

  SIO_TRACE (("LAN_lookup_pending"));

  /* Scan pending list section
   */
  for (i = arp_first_pending; i < arp_top_pending; i++)
      if (arp_list[i].ip == ip)
         return (TRUE);
  return (FALSE);
}

/**
 * Check ARP entries for timeout.
 *
 * This function runs through the 'pending' section of the ARP table and
 * checks for entries that have either expired or require a re-send.
 * The 'dynamic' entries are checked for expiry if requested.
 */
static void arp_check_timeouts (BOOL check_dynamic_entries)
{
  struct arp_entry *ae;
  int    i;

  SIO_TRACE (("arp_check_timeouts"));

  /* Check pending entries for retansmit & expiry
   */
  for (i = arp_first_pending; i < arp_top_pending; i++)
  {
    ae = &arp_list[i];

    /* If entry has expired (without being resolved): kill it
     */
    if (chk_timeout(ae->expiry))
    {
      if (i > arp_first_pending)
      {
        ae->flags &= ~ARP_INUSE;
        arp_move_entry (i--, arp_first_pending++); /* fill hole */
        continue; /* backed 'i' up a step, now re-check 'new' current entry */
      }
      ++arp_first_pending;
    }
    /* If time for a retransmission: do it & restart timeout
     */
    else if (chk_timeout(ae->retransmit_to))
    {
      /* If request fails, we try again a little sooner
       */
      ae->retransmit_to = set_timeout (arp_send_request(ae->ip) ?
                                       arp_rexmit_to : arp_rexmit_to / 4);
    }
  }

  /* Check dynamic entries for expiry
   */
  if (!check_dynamic_entries)
     return;

  for (i = arp_first_dynamic; i < arp_top_dynamic; i++)
  {
    ae = &arp_list[i];

    if (chk_timeout(ae->expiry))  /* entry has expired: kill it */
    {
      if (i < --arp_top_dynamic)
      {
        ae->flags &= ~ARP_INUSE;
        arp_move_entry (i--, arp_top_dynamic); /* fill hole */
        /* backed 'i' up a step, now re-check 'new' current entry */
      }
    }
  }
}


/**
 * ROUTE (& REDIRECT) HANDLING.
 */

/* Route table internal structure:
 * \verbatim
 *
 * Index pointers       Route cache table
 * --------------       -----------------
 *
 * (top_of_cache) ->
 *  (= top_pending)   ----------------------
 *                    | ...
 *                    | PENDING ROUTE ENTRIES (ARPing gateway - no reply yet)
 * first_pending ->   | ... grow downwards (into free slots)
 *  (= top_free)      ----------------------
 *                    | ...
 *                    | ...
 *                    | FREE SLOTS
 *                    | ...
 * first_free ---->   | ...
 *  (= top_dynamic)   ----------------------
 *                    | ... grow upwards (into free slots)
 *                    | ...
 *                    | "DYNAMIC" ROUTE ENTRIES
 * first_dynamic ->   | ...
 *                    ----------------------
 * \endverbatim
 *
 * \note
 *  - "Top" means last entry + 1.
 *  - The entries inside each section are not ordered in any way.
 *  - The route cache only holds entries of hosts that are OUTSIDE of our LAN.
 *  - The entries time out when the gateways ARP cache entry times out.
 */

/*!\struct route_entry
 * Route table.
 */
struct route_entry  {
       DWORD  host_ip; /* when connection to this host ... */
       DWORD  gate_ip; /* ... we use this gateway */
     };

#define route_top_of_cache    32
#define route_top_pending     route_top_of_cache
#define route_top_free        route_first_pending
#define route_top_dynamic     route_first_free
#define route_first_dynamic   0

static int route_first_pending = route_top_pending;
static int route_first_free    = 0;

static struct route_entry route_list [route_top_of_cache];

static __inline void route_move_entry (int to_index, int from_index)
{
  memcpy (&route_list[to_index], &route_list[from_index],
          sizeof(struct route_entry));
}

static BOOL route_makeNewSlot (DWORD host_ip, DWORD gate_ip)
{
  struct route_entry *re;

  /* We assume IP was already checked for, otherwise we would add it twice.
   * Check where we have room.
   */
  if (route_first_free < route_top_free)
  {
    /* Ok, free slots available */
  }
  else if (route_first_dynamic < route_top_dynamic)
  {
    /* Slaughter first dynamic entry, as new entry probably is more important
     */
    if (route_first_dynamic < --route_top_dynamic)
       route_move_entry (route_first_dynamic, route_top_dynamic);
    outsnl (_LANG("Route table overflow"));
  }
  else
  {
    outsnl (_LANG("Route table full"));
    return (FALSE); /* Nothing we can do - list is full of pending entries */
  }

  /* Put the new entry in
   */
  re = &route_list [--route_first_pending];
  re->host_ip = host_ip; /* when connection to this host ... */
  re->gate_ip = gate_ip; /* ... use this gateway */
  return (TRUE);
}

/**
 * This should probably go in pcconfig.c
 */
static BOOL is_on_LAN (DWORD ip)
{
  return (((ip ^ my_ip_addr) & sin_mask) == 0);
}

/**
 * Register a new host as gateway. Called on ICMP-redirects.
 *
 * Redirects are sent from a gateway/router, telling us that a
 * different gateway is better suited to connect to the specified
 * host than the one we were using.
 */
BOOL _arp_register (DWORD use_this_gateway_ip, DWORD for_this_host_ip)
{
  int i;

  SIO_TRACE (("_arp_register"));

  /* Only makes sense if ("old") host is outside of our LAN,
   * and ("new") gateway is on LAN.
   */
  if (!is_on_LAN (use_this_gateway_ip) || is_on_LAN (for_this_host_ip))
     return (FALSE);

  /* See if this guy is in our dynamic table
   */
  for (i = route_first_dynamic; i < route_top_dynamic; i++)
  {
    struct route_entry *re = &route_list[i];

    if (re->host_ip != for_this_host_ip)
       continue;

    if (re->gate_ip == use_this_gateway_ip)
       return (TRUE); /* Already done */

    if (LAN_lookup (use_this_gateway_ip, NULL))
    {
      re->gate_ip = use_this_gateway_ip;
      return (TRUE);  /* New gateway is already in ARP cache, done */
    }

    if (!LAN_start_lookup (use_this_gateway_ip))
    {
      outsnl (_LANG ("Unable to add redirect to ARP cache"));
      return (FALSE); /* ARP table full */
    }

    /* Kill 'old' dynamic entry, fill hole
     */
    if (i < --route_top_dynamic)
       route_move_entry (i, route_top_dynamic);

    /* Add new request, the new dynamic slot will be created when the
     * gateway ARP reply comes
     */
    return route_makeNewSlot (use_this_gateway_ip, for_this_host_ip);
  }

  /* Note: We do not check the pending section, as the gateway sending
   * the redirect could not really know the best route to a host that
   * we have not yet even started to connect to. Redirects referring
   * to a pending entry could be some sort of redirect attack.
   */
  return (FALSE);
}

/**
 * Gets MAC of gateway needed to reach the given host.
 */
static BOOL route_lookup (DWORD host_ip, eth_address *eth)
{
  int i;

  SIO_TRACE (("route_lookup"));

  /* 1st, we need to find the gateway entry for the specified host
   * in our route table
   */
  for (i = route_first_dynamic; i < route_top_dynamic; i++)
  {
    if (route_list[i].host_ip != host_ip)
       continue;

    /* 2nd, the gateway needs to be in the ARP table
     */
    return LAN_lookup (route_list[i].gate_ip, eth);
  }
  return (FALSE); /* host not here */
}

/**
 * Returns TRUE if the lookup of the gateway (assigned to the
 * supplied host) is still pending.
 */
static BOOL route_lookup_pending (DWORD host_ip)
{
  int i;

  SIO_TRACE (("route_lookup_pending"));

  /* Scan our pending list for the supplied host ip
   */
  for (i = route_first_pending; i < route_top_pending; i++)
  {
    if (route_list [i].host_ip == host_ip)
       return (TRUE);
  }
  return (FALSE); /* host not here */
}

/**
 * Start a route lookup.
 */
static BOOL route_start_lookup (DWORD host_ip)
{
  DWORD first_gate_ip;
  int   i;

  SIO_TRACE (("route_start_lookup"));

  /* Check if we already have an entry anywhere for this host
   */
  for (i = route_first_pending; i < route_top_pending; i++)
      if (route_list [i].host_ip == host_ip)
         return (TRUE);   /* Already here */

  for (i = route_first_dynamic; i < route_top_dynamic; i++)
      if (route_list [i].host_ip == host_ip)
         return (TRUE);   /* Already here */

  /* Abort if we don't have any gateways
   */
  if (gate_top <= 0)
  {
    outsnl (_LANG ("No gateways defined."));
    return (FALSE);
  }

  /* Find the first 'fitting' gateway
   */
  first_gate_ip = 0; /* we remember the first gateway IP that fits */

  for (i = 0; i < gate_top; i++)
  {
    struct gate_entry *gw = &gate_list [i];

    if (/* !! sin_mask != 0xFFFFFFFFUL && */ !is_on_LAN(gw->gate_ip))
       continue;

    if ((gw->mask & host_ip) != gw->subnet)
       continue;

    if (gw->is_dead)
       continue;

    if (!LAN_start_lookup (gw->gate_ip))
    {
      outsnl (_LANG ("Unable to add gateway to ARP cache"));
      return (FALSE); /* ARP table full, no point in going on right now */
    }
    first_gate_ip = gw->gate_ip; /* We start with this guy */
    break;
  }

  /* Abort if we didn't find anybody at all to ARP, or all ARPs failed
   */
  if (first_gate_ip == 0)
  {
    outsnl (_LANG ("No matching gateway"));
    return (FALSE);
  }

  /* Create a new route cache slot with our guy
   */
  route_makeNewSlot (host_ip, first_gate_ip);
  return (TRUE);
}


/**
 * Periodic route checker.
 * Run through all pending entries and check if an attempt to
 * reach a gateway was successfull, or has timed-out.
 * If the attempt timed-out, we try the next fitting gateway.
 * If there are no more fitting gateways to try, the connect has failed
 */
static void route_check_timeouts (BOOL check_dynamic_entries)
{
  int i, j;

  /* Check our pending entries
   */
  for (i = route_first_pending; i < route_top_pending; i++)
  {
    struct route_entry *re = &route_list [i];

    /* Was the ARP lookup able to resolve the gateway IP?
     */
    if (LAN_lookup (re->gate_ip, NULL))
    {
      /* Success - move route entry from pending to dynamic list
       */
      struct route_entry temp = *re; /* Make a copy so we can safely delete
                                      * the pending entry
                                      */
      if (i > route_first_pending)
         route_move_entry (i--, route_first_pending);    /* fill hole */
         /* (i-- to "re"check new current entry) */

      ++route_first_pending;              /* remove from pending list */
      route_list [route_top_dynamic++] = temp; /* add to dynamic list */
    }
    /* Is the ARP lookup still pending? -> Keep waiting
     */
    else if (LAN_lookup_pending (re->gate_ip))
    {
      /* Do nothing */
    }
    /* The ARP lookup failed, we try the next possible gateway
     */
    else
    {
      /* Find the gateway that was tried last (the one that just timed out)
       */
      BOOL foundLastGateway = FALSE;
      BOOL foundNextGateway = FALSE;

      for (j = 0; j < gate_top; j++)
      {
        if (gate_list[j].gate_ip != re->gate_ip)
           continue;
        foundLastGateway = TRUE;
        break;
      }

      if (!foundLastGateway)
         j = -1; /* If search failed, we try the first gateway "again". */

      /* Now we look for the next gateway that could be used
       */
      while (++j < gate_top)
      {
        struct gate_entry *gw = &gate_list [j];

        if (/* !! sin_mask != 0xFFFFFFFFUL && */ !is_on_LAN(gw->gate_ip))
           continue;

        if ((gw->mask & re->host_ip) != gw->subnet)
           continue;

        if (!LAN_start_lookup (gw->gate_ip))
           break;                  /* No room in ARP table, fail */

        re->gate_ip = gw->gate_ip; /* Ok, now we try this gateway */
        foundNextGateway = TRUE;
        break;
      }

      /* No more gateways to try, hence lookup failed, kill entry
       */
      if (!foundNextGateway)
      {
        if (i > route_first_pending)
           route_move_entry (i--, route_first_pending); /* fill hole */
        ++route_first_pending;
      }
    }
  }

  /* Check our dynamic list for expired entries
   */
  if (check_dynamic_entries)
  {
    for (i = route_first_dynamic; i < route_top_dynamic; i++)
    {
      if (LAN_lookup (route_list [i].gate_ip, NULL))
         continue; /* Still in ARP cache - ok */

      /* This guy has expired, kill from list
       */
      if (i < --route_top_dynamic)
      {
        route_move_entry (i--, route_top_dynamic); /* fill hole */
        /* (i backed up a step so 'new' slot is rechecked) */
      }
    }
  }
}


/**
 * "PUBLIC" ARP/ROUTE FUNCTIONS.
 *
 * These are the main functions visible from outside the module.
 * Most of them simply check if the destination host IP is on the
 * LAN or needs to be routed over a gateway, and then relay on to
 * the apropriate LAN_...() or route_...() function.
 */

#if !defined(USE_UDP_ONLY)
/**
 * Initialise an ARP lookup.
 * This is called by 'higher' routines to start an ARP request.
 * This function is non-blocking, i.e. returns 'immediately'.
 */
BOOL arp_start_lookup (DWORD ip)
{
  SIO_TRACE (("arp_start_lookup"));

  if (_pktserial)     /* Skip if using serial driver */
     return (TRUE);

  if (is_on_LAN(ip))
     return LAN_start_lookup (ip);
  return route_start_lookup (ip);
}


/**
 * Lookup MAC-address of 'ip'.
 * \retval TRUE MAC for 'ip' is known.
 */
BOOL arp_lookup (DWORD ip, eth_address *eth)
{
  SIO_TRACE (("arp_lookup"));

  /* Check if serial driver, return null MAC
   */
  if (_pktserial)
  {
    if (eth)
       memset (eth, 0, sizeof(*eth));
    return (TRUE);
  }

  if (_ip4_is_local_addr(ip))
  {
    if (eth)
       memcpy (eth, _eth_addr, sizeof(*eth));
    return (TRUE);
  }

  if (is_on_LAN(ip))
     return LAN_lookup (ip, eth);
  return route_lookup (ip, eth);
}


/**
 * An ARP-lookup timeout check.
 * \retval TRUE  The lookup is currently underway ("pending").
 * \retval FALSE The IP has either been resolved (arp_lookup() == TRUE),
 *         or the lookup has timed out, i.e. the host is unreachable.
 */
BOOL arp_lookup_pending (DWORD ip)
{
  SIO_TRACE (("arp_lookup_pending"));

  if (is_on_LAN(ip))
     return LAN_lookup_pending (ip);
  return route_lookup_pending (ip);
}

/**
 * Lookup fixed MAC-address of 'ip'.
 * \retval TRUE Supplied 'ip' has a fixed MAC entry.
 */
BOOL arp_lookup_fixed (DWORD ip, eth_address *eth)
{
  int i;

  SIO_TRACE (("arp_lookup_fixed"));

  if (_pktserial)
  {
    if (eth)
       memset (eth, 0, sizeof(*eth));
    return (TRUE);
  }

  if (!is_on_LAN(ip))  /* We only have/need a LAN version */
     return (FALSE);

  /* Scan fixed table section
   */
  for (i = arp_first_fixed; i < arp_top_fixed; i++)
  {
    if (arp_list[i].ip != ip)
       continue;
    if (eth)
       memcpy (eth, arp_list[i].hardware, sizeof(*eth));
    return (TRUE);
  }
  return (FALSE);
}
#endif  /* USE_UDP_ONLY */


/**
 * The blocking lookup function visible to higher functions.
 */
BOOL _arp_resolve (DWORD ip, eth_address *eth)
{
  WORD   brk_mode;
  BOOL (*lookup) (DWORD, eth_address*);
  BOOL (*start_lookup) (DWORD);
  BOOL (*pending_lookup) (DWORD);
  BOOL   rc = FALSE;

  SIO_TRACE (("_arp_resolve"));

  /* Check if serial driver, return null MAC
   */
  if (_pktserial)
  {
    if (eth)
       memset (eth, 0, sizeof(*eth));
    return (TRUE);
  }

  /* Check if ip is local address, return own MAC address
   */
  if (_ip4_is_local_addr(ip))
  {
    if (eth)
       memcpy (eth, _eth_addr, sizeof(*eth));
    return (TRUE);
  }

  /* Quick-check if we have this guy in our cache.
   */
  if (is_on_LAN(ip))
  {
    lookup         = LAN_lookup;
    start_lookup   = LAN_start_lookup;
    pending_lookup = LAN_lookup_pending;
  }
  else
  {
    lookup         = route_lookup;
    start_lookup   = route_start_lookup;
    pending_lookup = route_lookup_pending;
  }

  if ((*lookup)(ip, eth))
     return (TRUE);         /* Ok, done */

  /* Put out the request for the MAC
   */
  if (!(*start_lookup)(ip))
  {
    TCP_CONSOLE_MSG (2, ("%s (%d): %s() failed\n",
                     __FILE__, __LINE__, (start_lookup == LAN_start_lookup) ?
                     "LAN_start_lookup" : "route_start_lookup"));
    return (FALSE);  /* Request failed, resolve doomed */
  }

  NEW_BREAK_MODE (brk_mode, 1);

  /* Now busy-wait until reply is here or timeout (or Ctrl-C)
   */
  do
  {
    tcp_tick (NULL);   /* will call our daemon to timeout/retran */
    arp_daemon();      /* added for faster lookup */

    if ((*lookup)(ip, eth))
    {
      rc = TRUE;
      break;
    }
    WATT_YIELD();

    if (_watt_cbroke)
    {
      _watt_cbroke = 0;
      break;
    }
  }
  while ((*pending_lookup)(ip));

  OLD_BREAK_MODE (brk_mode);
  return (rc);
}


/**
 * Add given IP/Ether address to ARP-cache.
 * \note 'ip' is on host order.
 */
BOOL _arp_add_cache (DWORD ip, const void *eth, BOOL expires)
{
  struct arp_entry *ae;

  SIO_TRACE (("_arp_add_cache"));

  if (!my_ip_addr && !expires)
  {
   /* If called from e.g. pcconfig, my_ip_addr may be 0 when using
    * DHCP. Allow adding fixed entries.
    */
  }
  else if (!is_on_LAN (ip))  /* Only makes sense if on our LAN. */
     return (FALSE);

  _arp_delete_cache (ip);   /* Kill it if already here somewhere */

  /* Now add to correct list
   */
  if (expires) /* dynamic list */
  {
    if (arp_first_free >= arp_top_free)  /* No free dynamic slots */
       return (FALSE);

    /* Fill new slot data
     */
    ae = &arp_list [arp_top_dynamic++];
    ae->ip = ip;
    memcpy (&ae->hardware, eth, sizeof(ae->hardware));
    ae->expiry = set_timeout (1000UL * arp_alive);
    ae->flags  = (ARP_INUSE | ARP_DYNAMIC);
  }
  else   /* fixed list */
  {
    /* Check if we have any free slots; make room if possible
     */
    if (arp_first_free >= arp_top_free) /* No more fixed slots? */
    {
      if (arp_first_dynamic >= arp_top_dynamic)
         return (FALSE);   /* No free AND no dynamic slots! */
      --arp_top_dynamic;   /* Kill the top dynamic slot to make room */
    }

    /* Roll dynamic entires up one slot to make room for the new fixed entry
     */
    if (arp_first_dynamic < arp_top_dynamic)
       arp_move_entry (arp_top_dynamic, arp_first_dynamic);
    ++arp_top_dynamic;

    /* Fill new slot data
     */
    ae = &arp_list [arp_top_fixed++]; /* implies ++arp_first_dynamic! */
    ae->ip    = ip;
    ae->flags = (ARP_INUSE | ARP_FIXED);
    memcpy (&ae->hardware, eth, sizeof(ae->hardware));
  }
  return (TRUE);
}


/**
 * Delete given 'ip' address from ARP-cache (dynamic or fixed).
 * \note 'ip' is on host order.
 */
BOOL _arp_delete_cache (DWORD ip)
{
  int i;

  SIO_TRACE (("_arp_delete_cache"));

  /* Remove from dynamic list if present
   */
  for (i = arp_first_dynamic; i < arp_top_dynamic; i++)
  {
    if (arp_list[i].ip != ip)
       continue;

    if (i < --arp_top_dynamic)
       arp_move_entry (i, arp_top_dynamic); /* fill hole */
    arp_list[i].flags &= ~ARP_INUSE;
    return (TRUE);
  }

  /* Remove from fixed list if present
   */
  for (i = arp_first_fixed; i < arp_top_fixed; i++)
  {
    if (arp_list[i].ip != ip)
       continue;

    if (i < --arp_top_fixed)
       arp_move_entry (i, arp_top_fixed); /* fill hole */

    /* Do we have any dynamic entries we need to roll down one slot?
     * arp_first_dynamic same as arp_top_fixed, already implicity
     * decremented above!
     */
    if (arp_first_dynamic < --arp_top_dynamic)
       arp_move_entry (arp_first_dynamic, arp_top_dynamic);
    arp_list[i].flags &= ~ARP_INUSE;
    return (TRUE);
  }

  /* Remove from pending list if present
   */
  for (i = arp_first_pending; i < arp_top_pending; i++)
  {
    if (arp_list[i].ip != ip)
       continue;

    if (i > arp_first_pending)
       arp_move_entry (i, arp_first_pending); /* fill hole */
    ++arp_first_pending;
    arp_list[i].flags &= ~ARP_INUSE;
    return (TRUE);
  }
  return (FALSE); /* Didn't have it in cache */
}

/**
 * ARP "background" daemon.
 * Calls timeout-checks / retransmitters.
 *
 * We don't check the dynamic entries for a timeout on every call,
 * once a second is plenty enough.
 */
static void arp_daemon (void)
{
  static BOOL  check_dynamic       = TRUE;
  static DWORD check_dynamic_timer = 0UL;

  arp_check_timeouts   (check_dynamic);
  route_check_timeouts (check_dynamic);

  if (check_dynamic)
  {
#if defined(USE_DEAD_GWD) /* check dead gateways if we have >1 default GW */
    if (dead_gw_detect)
    {
      if (_arp_have_default_gw() <= 1)
           dead_gw_detect = FALSE;
      else check_dead_gw();
    }
#endif

    /* Preset check_dynamic for next call
     */
    check_dynamic_timer = set_timeout (1000UL);
    check_dynamic = FALSE;
  }
  else if (chk_timeout(check_dynamic_timer))
  {
    check_dynamic = TRUE;
  }
}

/**
 * Parser for "\c ARP.xx" keywords in "\c WATTCP.CFG".
 */
static void (*prev_cfg_hook) (const char*, const char*);

static void arp_parse (const char *name, const char *value)
{
  static const struct config_table arp_cfg[] = {
         { "TIMEOUT",       ARG_ATOI, (void*)&arp_timeout    },
         { "RETRANS_TO",    ARG_ATOI, (void*)&arp_rexmit_to  },
         { "ALIVE",         ARG_ATOI, (void*)&arp_alive      },
         { "NUM_CACHE",     ARG_ATOI, (void*)&arp_num_cache  },
         { "DEAD_GW_DETECT",ARG_ATOI, (void*)&dead_gw_detect },
         { "GRATIOTOUS",    ARG_ATOI, (void*)&arp_gratiotous },
         { NULL,            0,        NULL                   }
       };
  if (!parse_config_table(&arp_cfg[0], "ARP.", name, value) && prev_cfg_hook)
     (*prev_cfg_hook) (name, value);
}

#ifdef NOT_YET
static void (*prev_post_hook) (void);

static void arp_alloc (void)
{
  if (arp_num_cache < 10)
      arp_num_cache = 10;
  arp_list = calloc (sizeof(struct arp_entry), arp_num_cache);
  if (!arp_list)
  {
    outsnl (_LANG("Fatal: failed to allocate ARP-cache"));
    exit (-1);
  }
  if (prev_post_hook)
    (*prev_post_hook)();
}
#endif

/**
 * Setup config-table parse function and add background ARP deamon.
 */
void _arp_init (void)
{
  SIO_TRACE (("_arp_init"));
  addwattcpd (arp_daemon);

#ifdef NOT_YET
  prev_post_hook  = _watt_post_hook;
  _watt_post_hook = arp_alloc;
#endif
  prev_cfg_hook   = usr_init;
  usr_init        = arp_parse;
}


/**
 * Receive ARP handler.
 * This processes incoming 'raw' ARP packets supplied by tcp_tick().
 */
BOOL _arp_handler (const arp_Header *ah, BOOL brdcast)
{
  const eth_address *eth;
  WORD  hw_needed = intel16 (_eth_get_hwtype(NULL,NULL));
  DWORD src, dst;
  BOOL  do_reply = FALSE;
  int   i;

  SIO_TRACE (("_arp_handler"));

  DEBUG_RX (NULL, ah);

  if (ah->hwType   != hw_needed ||  /* wrong hardware type, */
      ah->protType != IP4_TYPE)     /* or not IPv4-protocol */
     return (FALSE);

#if 0
  if (ah->hwAddrLen    != sizeof(mac_address) ||
      ah->protoAddrLen != sizeof(ip))
     return (FALSE);
#endif

#if defined(USE_SECURE_ARP)
  if (_sarp_recv_hook)
     return (*_sarp_recv_hook) ((const struct sarp_Packet*)ah);
#endif

  /* Does someone else want our Ethernet address?
   */
  if (ah->opcode == ARP_REQUEST)
  {
    src = intel (ah->srcIPAddr);
    dst = intel (ah->dstIPAddr);

    if (_ip4_is_local_addr(dst) &&
        !_ip4_is_multicast(dst) &&
        !_ip4_is_loopback_addr(dst))
       do_reply = TRUE;

    if (src == my_ip_addr && _pkt_rxmode <= RXMODE_BROADCAST &&
        memcmp(&_eth_addr,&ah->srcEthAddr,_eth_mac_len))
    {
      TCP_CONSOLE_MSG (1, ("Address conflict from %s (%s)\n)",
                       _inet_ntoa(NULL,src),
                       MAC_address(&ah->srcEthAddr)));
    }

    /* Prevent anti-sniffers detecting us if we're not in normal rx-mode
     */
    if (_pkt_rxmode > RXMODE_BROADCAST &&
        memcmp(MAC_DST(ah),_eth_brdcast,sizeof(_eth_brdcast))) /* not bcast */
       do_reply = FALSE;

    if (do_reply)
       _arp_reply (&ah->srcEthAddr, ah->srcIPAddr, ah->dstIPAddr);
  }

  /* See if the senders IP & MAC is anything we can use
   */
  src = intel (ah->srcIPAddr);
  eth = &ah->srcEthAddr;

  /* Is this the awaited reply to a pending entry?
   */
  for (i = arp_first_pending; i < arp_top_pending; i++)
  {
    struct arp_entry *ae;

    if (ah->opcode != ARP_REPLY || arp_list[i].ip != src)
       continue;

    /* Remove from pending list
     */
    if (i > arp_first_pending)
       arp_move_entry (i, arp_first_pending); /* fill 'hole' */
    ++arp_first_pending;
    arp_list[i].flags &= ~ARP_PENDING;
    arp_list[i].flags &= ~ARP_INUSE;

    /* fill new dynamic entry
     * (at least one slot is free because we just freed a pending slot)
     */
    ae = &arp_list [arp_top_dynamic++];
    ae->ip    = src;
    ae->flags = (ARP_INUSE | ARP_DYNAMIC);

    memcpy (&ae->hardware, eth, sizeof(*eth));
    ae->expiry = set_timeout (1000UL * arp_alive);
    return (TRUE);          /* ARP reply was useful */
  }

  /* Or is this a 'refresher' of a dynamic entry?
   * We'll use both ARP_REQUEST and ARP_REPLY to refresh.
   */
  for (i = arp_first_dynamic; i < arp_top_dynamic; i++)
  {
    BOOL  equal;
    DWORD timeout;

    if (arp_list[i].ip != src)
       continue;

    /* This could also be an 'ARP poisoning attack', where an attacker is
     * trying to slip us a fake MAC address.
     * Knowing that, we check if the MAC address has changed, and if so
     * prematurely expire the entry. We will re-request it when we need it.
     * If the MAC address is 'still' the same, we just restart the timeout.
     */
    equal = (memcmp(&arp_list[i].hardware, eth, sizeof(*eth)) == 0);

    /* if poisoned, we give the 'real guy' 500 ms grace to reclaim his MAC ;)
     */
    timeout = (equal ? (1000UL * arp_alive) : 500UL);
    arp_list[i].expiry = set_timeout (timeout);
    return (TRUE);    /* ARP reply was useful */
  }

  /* Add to cache if we replied.
   */
  if (do_reply)
  {
    _arp_add_cache (src, &ah->srcEthAddr, TRUE);
    return (TRUE);
  }

  /* Most TCP stacks add any 'sniffed' ARP replies to their cache in case
   * they're needed later. But what are the odds? :)
   * Anyway, this could quickly fill the table with 'junk' entries on
   * heavily populated LANs; plus it makes us vulnerable to ARP-flooding
   * attacks ... so it's probably wiser to just ignore them.
   */
  ARGSUSED (brdcast);
  return (FALSE);  /* i.e. not handled */
}


#if defined(USE_DHCP)
/**
 * Used by DHCP initialisation.
 * Do an ARP resolve on our own IP address to check if someone
 * else is replying. Return non-zero if someone replied.
 * \note Blocks waiting for reply or timeout.
 */
BOOL _arp_check_own_ip (eth_address *other_guy)
{
  DWORD save = my_ip_addr;
  BOOL  rc;

  SIO_TRACE (("_arp_check_own_ip"));

  my_ip_addr = 0;
  memset (other_guy, 0, sizeof(*other_guy));
  rc = _arp_resolve (save, other_guy);
  my_ip_addr = save;

  if (rc && memcmp(other_guy,(const void*)"\0\0\0\0\0\0",sizeof(*other_guy)))
     return (FALSE);
  return (TRUE);
}
#endif


#if defined(USE_DEBUG)
/**
 * Return contents of ARP cache.
 *
 * 'max' entries are returned in 'arp[]' buffers. Only entries in-use
 * are returned.
 */
int _arp_list_cache (struct arp_entry *arp, int max)
{
  int i, num;

  for (i = num = 0; i < DIM(arp_list) && i < max; i++)
  {
    if (!(arp_list[i].flags & ARP_INUSE))
       continue;

    arp->ip     = arp_list[i].ip;
    arp->expiry = arp_list[i].expiry;
    arp->flags  = arp_list[i].flags;
    memcpy (arp->hardware, arp_list[i].hardware, sizeof(eth_address));
    arp++;
    num++;
  }
  return (num);
}

/**
 * Check for multiple default gateways.
 *
 * Return FALSE if we have none or more than 1.
 */
BOOL _arp_check_gateways (void)
{
  int i, num = 0;

  SIO_TRACE (("_arp_check_gateways"));

  /* Send a gratiotous ARP. Don't if already done DHCP_arp_check().
   */
#if defined(USE_DHCP)
  if (dhcp_did_gratuitous_arp)
     arp_gratiotous = FALSE;
#endif

  if (arp_gratiotous)
     _arp_reply (NULL, intel(IP_BCAST_ADDR), intel(my_ip_addr));

  for (i = 0; i < gate_top; i++)
      if (gate_list[i].subnet == 0UL)
         num++;
  return (num == 1);
}

/**
 * Debug-dump of configured gateways, route table and ARP cache.
 * Redone + moved here from pcdbug.c for encapsulation reasons
 * GvB 2002-09
 */
void _arp_debug_dump (void)
{
  DWORD now = set_timeout (0UL);
  int   i;

  /* Gateways
   */
  if (!dbug_printf ("Gateway list:\n"))
     return;

  if (gate_top == 0)
  {
    dbug_printf ("  --none--\n");
  }
  else for (i = 0; i < gate_top; i++)
  {
    const struct gate_entry *gw = &gate_list[i];
    DWORD mask = gw->mask ? gw->mask : sin_mask;

    dbug_printf ("  #%03d: %-15s ", i, _inet_ntoa(NULL, gw->gate_ip));
    dbug_printf ("(network: %-15s ", _inet_ntoa(NULL, gw->subnet));
    dbug_printf ("mask: %s)\n"   , _inet_ntoa(NULL, mask));
  }

  /* Route table
   */
  dbug_printf ("\nRouting cache:\n"
               "------- top of cache -----------------------------------------------\n"
               "  (%03d) top of pending slots ---------------------------------------\n",
               route_top_pending);

  if (route_first_pending == route_top_pending)
  {
    dbug_printf ("        --none--\n");
  }
  else if (route_first_pending > route_top_pending)
  {
    dbug_printf ("  INDEX ERROR!\n");
  }
  else for (i = route_top_pending-1; i >= route_first_pending; i--)
  {
    const struct route_entry *re = &route_list [i];

    dbug_printf ("  #%03d: IP: %-15s -> gateway IP %-15s\n",
                 i, _inet_ntoa(NULL,re->host_ip),
                 _inet_ntoa(NULL,re->gate_ip));
  }

  dbug_printf ("- (%03d) top of free slots ------------------------------------------\n",
               route_top_free);

  if (route_first_free == route_top_free)
  {
    dbug_printf ("  --none--\n");
  }
  else if (route_first_free > route_top_free)
  {
    dbug_printf ("  INDEX ERROR!\n");
  }
  else if (route_top_free - route_first_free <= 3)
  {
    for (i = route_top_free-1; i >= route_first_free; i--)
        dbug_printf ("  #%03d: (free)\n", i);
  }
  else
  {
    dbug_printf ("  #%03d: (free)\n", route_top_free-1);
    dbug_printf ("   ...  (free)\n");
    dbug_printf ("  #%03d: (free)\n", route_first_free);
  }

  dbug_printf ("- (%03d) top of dynamic slots ---------------------------------------\n",
               route_top_dynamic);
  if (route_first_dynamic == route_top_dynamic)
  {
    dbug_printf ("        --none--\n");
  }
  else if (route_first_dynamic > route_top_dynamic)
  {
    dbug_printf ("  INDEX ERROR!\n");
  }
  else for (i = route_top_dynamic-1; i >= route_first_dynamic; i--)
  {
    const struct route_entry *re = &route_list [i];

    dbug_printf ("  #%03d: IP: %-15s -> gateway IP %-15s\n",
                 i, _inet_ntoa(NULL,re->host_ip),
                 _inet_ntoa(NULL,re->gate_ip));
  }
  dbug_printf ("------- bottom of cache --------------------------------------------\n");

  /* ARP table
   */
  dbug_printf ("\nARP cache:\n"
               "------- top of cache -----------------------------------------------\n"
               "  (%03d) top of pending slots ---------------------------------------\n",
               arp_top_pending);

  if (arp_first_pending == arp_top_pending)
  {
    dbug_printf ("        --none--\n");
  }
  else if (arp_first_pending > arp_top_pending)
  {
    dbug_printf ("  INDEX ERROR!\n");
  }
  else for (i = arp_top_pending-1; i >= arp_first_pending; i--)
  {
    const struct arp_entry *ae = &arp_list [i];

    dbug_printf ("  #%03d: IP: %-15s -> ??:??:??:??:??:??  expires in %ss\n",
                 i, _inet_ntoa(NULL, ae->ip), time_str(ae->expiry - now));
  }

  dbug_printf ("- (%03d) top of free slots ------------------------------------------\n",
               arp_top_free);
  if (arp_first_free == arp_top_free)
  {
    dbug_printf ("  --none--\n");
  }
  else if (arp_first_free > arp_top_free)
  {
    dbug_printf ("  INDEX ERROR!\n");
  }
  else if (arp_top_free - arp_first_free <= 3)
  {
    for (i = arp_top_free-1; i >= arp_first_free; i--)
        dbug_printf ("  #%03d: (free)\n", i);
  }
  else
  {
    dbug_printf ("  #%03d: (free)\n", arp_top_free-1);
    dbug_printf ("   ...  (free)\n");
    dbug_printf ("  #%03d: (free)\n", arp_first_free);
  }

  dbug_printf ("- (%03d) top of dynamic slots ---------------------------------------\n",
               arp_top_dynamic);
  if (arp_first_dynamic == arp_top_dynamic)
  {
    dbug_printf ("        --none--\n");
  }
  else if (arp_first_dynamic > arp_top_dynamic)
  {
    dbug_printf ("  INDEX ERROR!\n");
  }
  else for (i = arp_top_dynamic-1; i >= arp_first_dynamic; i--)
  {
    const struct arp_entry *ae = &arp_list [i];

    dbug_printf ("  #%03d: IP: %-15s -> %s  expires in %ss\n",
                 i, _inet_ntoa(NULL,ae->ip), MAC_address(&ae->hardware),
                 time_str(ae->expiry - now));
  }

  dbug_printf ("- (%03d) top of fixed slots -----------------------------------------\n",
               arp_top_fixed);
  if (arp_first_fixed == arp_top_fixed)
  {
    dbug_printf ("        --none--\n");
  }
  else if (arp_first_fixed > arp_top_fixed)
  {
    dbug_printf ("  INDEX ERROR!\n");
  }
  else for (i = arp_top_fixed-1; i >= arp_first_fixed; i--)
  {
    const struct arp_entry *ae = &arp_list [i];

    dbug_printf ("  #%03d: IP: %-15s -> %s\n",
                 i, _inet_ntoa(NULL,ae->ip), MAC_address(&ae->hardware));
  }
  dbug_printf ("------- bottom of cache --------------------------------------------\n");
}
#endif  /* USE_DEBUG */
