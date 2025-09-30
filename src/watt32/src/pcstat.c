/*!\file pcstat.c
 * Input/output statistics.
 */

/*  Update and printing of MAC/IP/ICMP/IGMP/UDP/TCP input/output
 *  statistics counters.
 *
 *  Copyright (c) 1997-2002 Gisle Vanem <giva@bgnett.no>
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. All advertising materials mentioning features or use of this software
 *     must display the following acknowledgement:
 *       This product includes software developed by Gisle Vanem
 *       Bergen, Norway.
 *
 *  THIS SOFTWARE IS PROVIDED BY ME (Gisle Vanem) AND CONTRIBUTORS ``AS IS''
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL I OR CONTRIBUTORS BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  G. Vanem <giva@bgnett.no> 1997
 */

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <net/ppp_defs.h>
#include <arpa/inet.h>

#include "wattcp.h"
#include "strings.h"
#include "sock_ini.h"
#include "pcconfig.h"
#include "pcicmp.h"
#include "pcqueue.h"
#include "pcsed.h"
#include "split.h"
#include "misc.h"
#include "timer.h"
#include "pcpkt.h"
#include "pcdbug.h"
#include "pppoe.h"
#include "pcstat.h"

int sock_stats (sock_type *sock, DWORD *days, WORD *inactive,
                WORD *cwindow, DWORD *avg, DWORD *sd)
{
  _tcp_Socket *s = &sock->tcp;

  if (s->ip_type != TCP_PROTO)
     return (0);

  if (days)     *days     = get_day_num() - start_day;
  if (inactive) *inactive = sock_inactive;
#if defined(USE_UDP_ONLY)
  if (cwindow)  *cwindow  = 0;
  if (avg)      *avg      = 0;
  if (sd)       *sd       = 0;
#else
  if (cwindow)  *cwindow  = s->cwindow;
  if (avg)      *avg      = s->vj_sa >> 3;
  if (sd)       *sd       = s->vj_sd >> 2;
#endif
  return (1);
}

#if !defined(USE_STATISTICS)
  void print_mac_stats (void)  {}
  void print_vjc_stats (void)  {}
  void print_pkt_stats (void)  {}
  void print_ip4_stats (void)  {}
  void print_ip6_stats (void)  {}
  void print_icmp_stats(void)  {}
  void print_icmp6_stats(void) {}
  void print_igmp_stats(void)  {}
  void print_udp_stats (void)  {}
  void print_tcp_stats (void)  {}
  void print_all_stats (void)  {}
  void reset_stats     (void)  {}

#else  /* rest of file */

struct macstat   macstats;
struct ipstat    ip4stats;
struct ip6stat   ip6stats;
struct udpstat   udpstats;
struct tcpstat   tcpstats;
struct icmpstat  icmpstats;
struct icmp6stat icmp6stats;
struct igmpstat  igmpstats;
struct pppoestat pppoestats;

void reset_stats (void)
{
  memset (&macstats,  0, sizeof(macstats));
  memset (&ip4stats,  0, sizeof(ip4stats));
  memset (&ip6stats,  0, sizeof(ip6stats));
  memset (&udpstats,  0, sizeof(udpstats));
  memset (&tcpstats,  0, sizeof(tcpstats));
  memset (&icmpstats, 0, sizeof(icmpstats));
  memset (&igmpstats, 0, sizeof(igmpstats));
  memset (&pppoestats,0, sizeof(pppoestats));
}

static __inline void show_stat (const char *name, DWORD num)
{
  if (num)
    (*_printf) ("      %-13s %10lu\n", name, num);
}

#if defined(HAVE_UINT64) && defined(USE_IPV6)
static __inline void show_stat64 (const char *name, uint64 num)
{
  if (num)
    (*_printf) ("      %-13s %10" U64_FMT "\n", name, num);
}
#endif

void print_mac_stats (void)
{
  (*_printf) ("MAC   input stats:\n"
              "      dropped %lu, waiting %d, looped %lu, non-IP recv %lu sent %lu\n"
              "      unknown types, %lu, LLC frames %lu, IP-recursions %lu",
              pkt_dropped(), pkt_waiting(), macstats.num_mac_loop,
              macstats.non_ip_recv, macstats.non_ip_sent,
              macstats.num_unk_type, macstats.num_llc_frames,
              macstats.num_ip_recurse);

#if defined(USE_FAST_PKT) || defined(WIN32)
  (*_printf) (" \n      wrong handle %lu, bad-sync %lu, too large %lu, too small %lu",
              macstats.num_wrong_handle, macstats.num_bad_sync,
              macstats.num_too_large, macstats.num_too_small);
#endif

  (*_printf) (" \nMAC   output stats:\n"
              "      Tx errors     %10lu %s\n"
              "      Tx retries    %10lu\n"
              "      Tx timeout    %10lu\n",
              macstats.num_tx_err, macstats.num_tx_err ? "****" : "",
              macstats.num_tx_retry, macstats.num_tx_timeout);
}

void print_arp_stats (void)
{
  (*_printf) ("ARP   Requests:     %10lu recv\n"
              "      Requests:     %10lu sent\n"
              "      Replies:      %10lu recv\n"
              "      Replies:      %10lu sent\n",
              macstats.arp.request_recv, macstats.arp.request_sent,
              macstats.arp.reply_recv, macstats.arp.reply_sent);
}

void print_rarp_stats (void)
{
#if defined(USE_RARP)
  DWORD total = macstats.rarp.request_recv +
                macstats.rarp.request_sent +
                macstats.rarp.reply_recv   +
                macstats.rarp.reply_sent;

  if (total > 0)
     (*_printf) ("RARP  Requests:     %10lu recv\n"
                 "      Requests:     %10lu sent\n"
                 "      Replies:      %10lu recv\n"
                 "      Replies:      %10lu sent\n",
                 macstats.rarp.request_recv, macstats.rarp.request_sent,
                 macstats.rarp.reply_recv, macstats.rarp.reply_sent);
#endif
}

void print_vjc_stats (void)
{
#if !defined(WIN32)
  struct slcompress vjc;

  if (stricmp(_pktdrvrname,"PPPD220F") &&   /* not using DOSPPP driver */
      stricmp(_pktdrvrname,"LADSoftPPP$"))  /* or LadSoft's lsppp */
     return;

  (*_printf) ("VJ compression:\n");

  if (!pkt_get_vjstats(&vjc))
  {
    (*_printf) ("      not available\n\n");
    return;
  }

  (*_printf) ("      Tx TCP packets: %lu, non-TCP: %lu\n",
              vjc.sls_o_tcp, vjc.sls_o_nontcp);

  (*_printf) ("      Rx error packets: %lu, tossed: %lu\n",
              vjc.sls_i_error, vjc.sls_i_tossed);

  (*_printf) ("      Rx runt packets: %lu, bad checksum: %lu\n",
               vjc.sls_i_runt, vjc.sls_i_badcheck);

  (*_printf) ("      connection state searches: %lu, misses: %lu\n",
              vjc.sls_o_searches, vjc.sls_o_misses);

  (*_printf) ("      oldest xmit in ring: %-9u\n", vjc.xmit_oldest);
  (*_printf) ("      compression state flags: 0x%X\n",  vjc.flags);

  (*_printf) ("      highest slot:         Tx %-5u Rx %u\n",
              vjc.tslot_limit, vjc.rslot_limit);

  (*_printf) ("      most recent id:       Tx %-5u Rx %u\n",
               vjc.xmit_current, vjc.recv_current);

  (*_printf) ("      uncompressed packets: Tx %-5lu Rx %lu\n",
              vjc.sls_o_uncompressed, vjc.sls_i_uncompressed);

  (*_printf) ("      compressed packets:   Tx %-5lu Rx %lu\n",
              vjc.sls_o_compressed, vjc.sls_i_compressed);

  if (!stricmp(_pktdrvrname,"PPPD220F"))  /* using DOSPPP driver */
  {
    struct cstate cstate;
    WORD   cs_ofs, rx_num = 0, tx_num = 0;

    (*_printf) ("  Connection states:\n");

    for (cs_ofs = vjc.cstate_ptr_tstate;
         pkt_get_cstate(&cstate,cs_ofs) && tx_num < vjc.tslot_limit;
         cs_ofs = cstate.cs_next, tx_num++)
        (*_printf) ("      Tx-id %2d: IP %15s -> %15s, TCP port %u -> %d\n",
                    cstate.cs_this,
                    inet_ntoa(*(struct in_addr*)&cstate.cs_ip.source),
                    inet_ntoa(*(struct in_addr*)&cstate.cs_ip.destination),
                    intel16(cstate.cs_tcp.srcPort),
                    intel16(cstate.cs_tcp.dstPort));

    for (cs_ofs = vjc.cstate_ptr_rstate;
         pkt_get_cstate(&cstate,cs_ofs) && rx_num < vjc.rslot_limit;
         cs_ofs = cstate.cs_next, rx_num++)
        (*_printf) ("      Rx-id %2d: IP %15s -> %15s, TCP port %u -> %d\n",
                    cstate.cs_this,
                    inet_ntoa(*(struct in_addr*)&cstate.cs_ip.source),
                    inet_ntoa(*(struct in_addr*)&cstate.cs_ip.destination),
                    intel16(cstate.cs_tcp.srcPort),
                    intel16(cstate.cs_tcp.dstPort));
    if (rx_num + tx_num == 0)
       (*_printf) ("      not available\n");
  }
  (*_printf) (" \n");
#endif
}

void print_pkt_stats (void)
{
  struct PktStats stat, total;

  (*_printf) ("Driver statistics:\n");
  if (!pkt_get_stats(&stat,&total))
  {
    (*_printf) ("      Not available\n");
    return;
  }

  (*_printf) ("      Rx: %9lu pkt, %10lu bytes, %6lu errors, %6lu lost  (this session)\n",
              stat.in_packets, stat.in_bytes, stat.in_errors, stat.lost);
  (*_printf) ("      Tx: %9lu pkt, %10lu bytes, %6lu errors\n",
              stat.out_packets, stat.out_bytes, stat.out_errors);

  (*_printf) ("      Rx: %9lu pkt, %10lu bytes, %6lu errors, %6lu lost  (since boot)\n",
              total.in_packets, total.in_bytes, total.in_errors, total.lost);
  (*_printf) ("      Tx: %9lu pkt, %10lu bytes, %6lu errors\n",
              total.out_packets, total.out_bytes, total.out_errors);
}

void print_pppoe_stats (void)
{
#if defined(USE_PPPOE)
  (*_printf) ("PPPoE stats:\n");
  show_stat ("Tx Discovery:", pppoestats.num_disc_sent);
  show_stat ("Rx Discovery:", pppoestats.num_disc_recv);
  show_stat ("Tx Session:",   pppoestats.num_sess_sent);
  show_stat ("Rx Session:",   pppoestats.num_sess_recv);
#endif
}

void print_ip4_stats (void)
{
  (*_printf) ("IP4   input stats:\n");
  show_stat ("total:",      ip4stats.ips_total);
  show_stat ("badsum:",     ip4stats.ips_badsum);
  show_stat ("badver:",     ip4stats.ips_badvers);
  show_stat ("short:",      ip4stats.ips_tooshort);
  show_stat ("frags:",      ip4stats.ips_fragments);
  show_stat ("fragdrop:",   ip4stats.ips_fragdropped);
  show_stat ("fragtimeout:",ip4stats.ips_fragtimeout);
  show_stat ("reassemble:", ip4stats.ips_reassembled);
  show_stat ("noproto:",    ip4stats.ips_noproto);
  show_stat ("delivered:",  ip4stats.ips_delivered);
  show_stat ("badoptions:", ip4stats.ips_badoptions);
  show_stat ("dropped:",    ip4stats.ips_idropped);

  (*_printf) ("IP4   output stats:\n");
  show_stat ("total:",      ip4stats.ips_rawout);
  show_stat ("noroute:",    ip4stats.ips_noroute);
  show_stat ("frags:",      ip4stats.ips_ofragments);
  show_stat ("dropped:",    ip4stats.ips_odropped);
}

void print_ip6_stats (void)
{
#if defined(HAVE_UINT64) && defined(USE_IPV6)
  (*_printf) ("IP6   input stats:\n");
  show_stat64 ("total:",     ip6stats.ip6s_total);
  show_stat64 ("badver:",    ip6stats.ip6s_badvers);
  show_stat64 ("short:",     ip6stats.ip6s_tooshort);
  show_stat64 ("frags:",     ip6stats.ip6s_fragments);
  show_stat64 ("fragdrop:",  ip6stats.ip6s_fragdropped);
  show_stat64 ("fragtime:",  ip6stats.ip6s_fragtimeout);
  show_stat64 ("reassemble:",ip6stats.ip6s_reassembled);
  show_stat64 ("delivered:", ip6stats.ip6s_delivered);
  show_stat64 ("badoptions:",ip6stats.ip6s_badoptions);

  (*_printf) ("IP6   output stats:\n");
  show_stat64 ("total:",     ip6stats.ip6s_rawout);
  show_stat64 ("noroute:",   ip6stats.ip6s_noroute);
  show_stat64 ("frags:",     ip6stats.ip6s_ofragments);
  show_stat64 ("dropped:",   ip6stats.ip6s_odropped);
#endif
}

void print_icmp6_stats (void)
{
#if defined(HAVE_UINT64) && defined(USE_IPV6)
  uint64 total_in  = 0;
  uint64 total_out = 0;
  int    i;

  (*_printf) ("ICMP6 input stats:\n");

  for (i = 0; i < DIM(icmp6stats.icp6s_inhist); i++)
      total_in += icmp6stats.icp6s_inhist[i];

  show_stat64 ("total:",       total_in);

  show_stat64 ("badsum:",     icmp6stats.icp6s_checksum);
  show_stat64 ("short:",      icmp6stats.icp6s_tooshort);
  show_stat64 ("badcode:",    icmp6stats.icp6s_badcode);

  (*_printf) ("ICMP6 output stats:\n");
  for (i = 0; i < DIM(icmp6stats.icp6s_outhist); i++)
      total_out += icmp6stats.icp6s_outhist[i];

  show_stat64 ("total:",       total_out);
  show_stat64 ("too big:",     icmp6stats.icp6s_opacket_too_big);
  show_stat64 ("exceed xmit:", icmp6stats.icp6s_otime_exceed_transit);
  show_stat64 ("exceed reasm:",icmp6stats.icp6s_otime_exceed_reassembly);
#endif
}

void print_icmp_stats (void)
{
  DWORD  total_in  = 0;
  DWORD  total_out = 0;
  int    i;
  static const char *types[ICMP_MAXTYPE+1] = {
              "echoreply:",
              "1?:",
              "2?:",
              "unreach:",
              "srcquench:",
              "redirect:",
              "6?:",
              "7?:",
              "echorequest:",
              "router adv:",
              "router sol:",
              "timex:",
              "parm prob:",
              "tstamp req:",
              "tstamp rep:",
              "info req:",
              "info rep:",
              "mask req:",
              "mask rep:"
            };

  (*_printf) ("ICMP  input stats:\n");
  for (i = 0; i < ICMP_MAXTYPE; i++)
      total_in += icmpstats.icps_inhist[i];
  show_stat ("total:", total_in);

  show_stat ("badsum:", icmpstats.icps_checksum);
  show_stat ("badcode:",icmpstats.icps_badcode);
  show_stat ("short:",  icmpstats.icps_tooshort);

  for (i = 0; i < ICMP_MAXTYPE; i++)
     show_stat (types[i], icmpstats.icps_inhist[i]);

  (*_printf) ("ICMP  output stats:\n");
  for (i = 0; i < ICMP_MAXTYPE; i++)
      total_out += icmpstats.icps_outhist[i];
  show_stat ("total:", total_out);

  for (i = 0; i < ICMP_MAXTYPE; i++)
     show_stat (types[i], icmpstats.icps_outhist[i]);
}

void print_igmp_stats (void)
{
#if defined(USE_MULTICAST)
  (*_printf) ("IGMP  input stats:\n");
  show_stat ("total:",        igmpstats.igps_rcv_total);
  show_stat ("badsum:",       igmpstats.igps_rcv_badsum);
  show_stat ("short:",        igmpstats.igps_rcv_tooshort);
  show_stat ("queries:",      igmpstats.igps_rcv_queries);
  show_stat ("queries (bad):",igmpstats.igps_rcv_badqueries);
  show_stat ("reports:",      igmpstats.igps_rcv_reports);
  show_stat ("  bad:",        igmpstats.igps_rcv_badreports);
  show_stat ("  group:",      igmpstats.igps_rcv_ourreports);

  (*_printf) ("IGMP  output stats:\n");
  show_stat ("reports:", igmpstats.igps_snd_reports);
#endif
}

void print_udp_stats (void)
{
  (*_printf) ("UDP   input stats:\n");
  show_stat ("total:",      udpstats.udps_ipackets);
  show_stat ("drops:",      udpstats.udps_hdrops);
  show_stat ("badsum:",     udpstats.udps_badsum);
  show_stat ("no service:", udpstats.udps_noport);
  show_stat ("broadcast:",  udpstats.udps_noportbcast);
  show_stat ("queue full:", udpstats.udps_fullsock);

  (*_printf) ("UDP   output stats:\n");
  show_stat ("total:",      udpstats.udps_opackets);
}

void print_tcp_stats (void)
{
#if !defined(USE_UDP_ONLY)
  (*_printf) ("TCP   input stats:\n");
  show_stat ("total:",       tcpstats.tcps_rcvtotal);
  show_stat ("drops:",       tcpstats.tcps_drops);
  show_stat ("badsum:",      tcpstats.tcps_rcvbadsum);
  show_stat ("badhofs:",     tcpstats.tcps_rcvbadoff);
  show_stat ("conn-okay:",   tcpstats.tcps_connects);
  show_stat ("conn-accept:", tcpstats.tcps_accepts);
  show_stat ("conn-drops:",  tcpstats.tcps_conndrops);
  show_stat ("inS pkts:",    tcpstats.tcps_rcvpack);
  show_stat ("inS bytes:",   tcpstats.tcps_rcvbyte);
  show_stat ("OoO pkts:",    tcpstats.tcps_rcvoopack);
  show_stat ("OoO bytes:",   tcpstats.tcps_rcvoobyte);
  show_stat ("ACK pkts:",    tcpstats.tcps_rcvackpack);
  show_stat ("ACK bytes:",   tcpstats.tcps_rcvackbyte);
  show_stat ("ACK nosent:",   tcpstats.tcps_rcvacktoomuch);
  show_stat ("dup ACK:",     tcpstats.tcps_rcvduppack);
  show_stat ("dup bytes:",   tcpstats.tcps_rcvdupbyte);
  show_stat ("Outside win:", tcpstats.tcps_rcvpackafterwin);
  show_stat ("pers-drop:",   tcpstats.tcps_persistdrop);

  (*_printf) ("TCP   output stats:\n");
  show_stat ("total:",       tcpstats.tcps_sndtotal);
  show_stat ("data pkts:",   tcpstats.tcps_sndpack);
  show_stat ("data bytes:",  tcpstats.tcps_sndbyte);
  show_stat ("conn-attempt:",tcpstats.tcps_connattempt);
  show_stat ("SYN/FIN/RST:", tcpstats.tcps_sndctrl);
  show_stat ("close/drops:", tcpstats.tcps_closed);
  show_stat ("OOB pkts:",    tcpstats.tcps_sndurg);
  show_stat ("ACK pkts:",    tcpstats.tcps_sndacks);
  show_stat ("ACK dly:",     tcpstats.tcps_delack);
  show_stat ("ACK win upd:", tcpstats.tcps_sndwinup);
  show_stat ("retrans to:",  tcpstats.tcps_rexmttimeo);
  show_stat ("keepalive pr:",tcpstats.tcps_keepprobe);
  show_stat ("keepalive to:",tcpstats.tcps_keeptimeo);
  show_stat ("RTTcache add:",tcpstats.tcps_cachedrtt);
  show_stat ("RTTcache get:",tcpstats.tcps_usedrtt);
#endif
}

void print_all_stats (void)
{
  int save = ctrace_on;

  ctrace_on = 0;

  print_vjc_stats();
  print_pkt_stats();
  print_mac_stats();

  if (!_pktserial)
  {
    print_arp_stats();
    print_rarp_stats();
    print_pppoe_stats();
  }

  print_ip4_stats();
  print_ip6_stats();
  print_icmp_stats();
  print_icmp6_stats();
  print_igmp_stats();
  print_udp_stats();
  print_tcp_stats();

  ctrace_on = save;
}

/*
 * Called from the link-layer input routine _eth_arrived().
 */
void update_in_stat (void)
{
  const struct pkt_split *ps;
  const eth_Header       *eth;
  const tok_Header       *tok;
  const ICMP_PKT         *icmp;
  BOOL  got_ip = FALSE;
  BYTE  type;

  for (ps = pkt_get_split_in(); ps->data; ps++)
  {
    switch (ps->type)
    {
      case TYPE_ETHER_HEAD:
           eth = (const eth_Header*) ps->data;
           if (!memcmp(eth->source,_eth_addr,sizeof(_eth_addr)))
              macstats.num_mac_loop++;
           break;

      case TYPE_TOKEN_HEAD:
           tok = (const tok_Header*) ps->data;
           if (!memcmp(tok->source,_eth_addr,sizeof(_eth_addr)))
              macstats.num_mac_loop++;
           break;

      case TYPE_LLC_HEAD:
           macstats.num_llc_frames++;
           break;

      case TYPE_ARP:
           {
             const arp_Header *arp = (const arp_Header*) ps->data;

             if (arp->opcode == ARP_REQUEST)
                macstats.arp.request_recv++;
             else if (arp->opcode == ARP_REPLY)
                macstats.arp.reply_recv++;
             break;
           }
#if defined(USE_RARP)
      case TYPE_RARP:
           {
             const rarp_Header *rarp = (const rarp_Header*) ps->data;

             if (rarp->opcode == RARP_REQUEST)
                macstats.rarp.request_recv++;
             else if (rarp->opcode == RARP_REPLY)
                macstats.rarp.reply_recv++;
             break;
           }
#endif

#if defined(USE_PPPOE)
      case TYPE_PPPOE_DISC:
           pppoestats.num_disc_recv++;
           break;

      case TYPE_PPPOE_SESS:
           pppoestats.num_sess_recv++;
           break;

      case TYPE_PPP_LCP:
      case TYPE_PPP_IPCP:
           /** \todo Count PPP protocols LCP and IPCP */
           break;
#endif

#if defined(USE_IPV6)
      case TYPE_IP6:
           ip6stats.ip6s_total++;
           got_ip = TRUE;
           break;

      case TYPE_IP6_HOP:
      case TYPE_IP6_IPV6:
      case TYPE_IP6_ROUTING:
      case TYPE_IP6_FRAGMENT:
      case TYPE_IP6_ESP:
      case TYPE_IP6_AUTH:
      case TYPE_IP6_ICMP:
      case TYPE_IP6_DEST:
           /** \todo Count these IPv6 protocols */
           break;
      case TYPE_IP6_NONE:
      case TYPE_IP6_UNSUPP:
           break; /* ignore */
#endif

      case TYPE_IP4:
           ip4stats.ips_total++;
           got_ip = TRUE;
           break;

      case TYPE_IP4_FRAG:
           got_ip = TRUE;
           break;

      case TYPE_ICMP:
           icmp = (const ICMP_PKT*) ps->data;
           type = icmp->unused.type;
           if (type <= ICMP_MAXTYPE)
              icmpstats.icps_inhist[(int)type]++;
           return;

      case TYPE_IGMP:
           igmpstats.igps_rcv_total++;
           return;

      case TYPE_UDP_HEAD:
           udpstats.udps_ipackets++;
           return;

      case TYPE_TCP_HEAD:
           tcpstats.tcps_rcvtotal++;
           return;

      default:
#if defined(USE_DEBUG) && 1  /* !! bug hunt */
           dbug_printf ("Unknown split-type %d\n", ps->type);
#endif
           macstats.num_unk_type++;
           break;
    }
  }
  if (!got_ip)
     macstats.non_ip_recv++;
}

/*
 * Called from the link-layer output routine _eth_send().
 */
void update_out_stat (void)
{
  const struct pkt_split *ps;
  const arp_Header       *arp;
  const in_Header        *ip;
  const ICMP_PKT         *icmp;
  BOOL  got_ip  = FALSE;
  BYTE  type, flags;
  DWORD ofs;

  for (ps = pkt_get_split_out(); ps->data; ps++)
  {
    switch (ps->type)
    {
      case TYPE_ARP:
           arp = (arp_Header*) ps->data;
           if (arp->opcode == ARP_REQUEST)
              macstats.arp.request_sent++;
           else if (arp->opcode == ARP_REPLY)
              macstats.arp.reply_sent++;
           break;

#if defined(USE_RARP)
      case TYPE_RARP:
           {
             const rarp_Header *rarp = (rarp_Header*) ps->data;

             if (rarp->opcode == RARP_REQUEST)
                macstats.rarp.request_sent++;
             else if (rarp->opcode == RARP_REPLY)
                macstats.rarp.reply_sent++;
           }
           break;
#endif

#if defined(USE_PPPOE)
      case TYPE_PPPOE_DISC:
           pppoestats.num_disc_sent++;
           break;

      case TYPE_PPPOE_SESS:
           pppoestats.num_sess_sent++;

      case TYPE_PPP_LCP:
      case TYPE_PPP_IPCP:
           /** \todo Count the PPP protocols LCP and IPCP */
           break;
#endif

#if defined(USE_IPV6)
      case TYPE_IP6:
           got_ip = TRUE;
           ip6stats.ip6s_rawout++;
           break;

      case TYPE_IP6_HOP:
      case TYPE_IP6_IPV6:
      case TYPE_IP6_ROUTING:
      case TYPE_IP6_FRAGMENT:
      case TYPE_IP6_ESP:
      case TYPE_IP6_AUTH:
      case TYPE_IP6_ICMP:
      case TYPE_IP6_DEST:
           /** \todo Count these IPv6 protocols */
           break;
#endif

      case TYPE_IP4:
           got_ip = TRUE;
           ip = (const struct in_Header*) ps->data;
           ip4stats.ips_rawout++;   /* count raw IP (fragmented or not) */

           ofs = intel16 (ip->frag_ofs);
           ofs = (ofs & IP_OFFMASK) << 3;
           if (ofs)
              return;  /* only fragments with ofs 0 (already counted) */
           break;

      case TYPE_ICMP:
           icmp = (const ICMP_PKT*) ps->data;
           type = icmp->unused.type;
           if (type <= ICMP_MAXTYPE)
              icmpstats.icps_outhist[(int)type]++;
           return;

#if defined(USE_MULTICAST)
      case TYPE_IGMP:
           {
             const IGMP_packet *igmp = (const IGMP_packet*) ps->data;

             if (igmp->version == 1 && igmp->type == IGMPv1_REPORT)
                igmpstats.igps_snd_reports++;
             return;
           }
#endif

      case TYPE_UDP_HEAD:
           udpstats.udps_opackets++;
           return;

#if !defined(USE_UDP_ONLY)
      case TYPE_TCP_HEAD:
           {
             const tcp_Header *tcp = (const tcp_Header*) ps->data;

             flags = tcp->flags & tcp_FlagMASK;
             tcpstats.tcps_sndtotal++;

             if (flags & (tcp_FlagSYN|tcp_FlagFIN|tcp_FlagRST))
                tcpstats.tcps_sndctrl++;
             else if (flags & tcp_FlagACK) /* ACK only flag */
                tcpstats.tcps_sndacks++;
             if (flags & tcp_FlagURG)
                tcpstats.tcps_sndurg++;
             if (flags & (tcp_FlagFIN|tcp_FlagRST))
                tcpstats.tcps_closed++;
           }
           break;

      case TYPE_TCP_DATA:
           if (ps->len > 0)
           {
             tcpstats.tcps_sndpack++;
             tcpstats.tcps_sndbyte += ps->len;
           }
           return;   /* nothing behind this */
#endif

      default: ;    /* squelch gcc warning */
    }
  }

 if (!got_ip)
    macstats.non_ip_sent++;
}
#endif  /* USE_STATISTICS */

