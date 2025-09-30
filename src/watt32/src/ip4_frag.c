/*!\file ip4_frag.c
 * IP4 de-fragmenter.
 */

/*
 *  Packet de-fragmentation code for WATTCP
 *  Written by and COPYRIGHT (c)1993 to Quentin Smart
 *                               smart@actrix.gen.nz
 *  all rights reserved.
 *
 *    This software is distributed in the hope that it will be useful,
 *    but without any warranty; without even the implied warranty of
 *    merchantability or fitness for a particular purpose.
 *
 *    You may freely distribute this source code, but if distributed for
 *    financial gain then only executables derived from the source may be
 *    sold.
 *
 *  Murf = Murf@perftech.com
 *  other fragfix = mdurkin@tsoft.net
 *
 *  Based on RFC815
 *
 *  Code used to use pktbuf[] as reassembly buffer. It now allocates
 *  a "bucket" dynamically. There are MAX_IP_FRAGS buckets to handle
 *  at the same time.
 *
 *  G.Vanem 1998 <giva@bgnett.no>
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <arpa/inet.h>

#include "wattcp.h"
#include "strings.h"
#include "language.h"
#include "misc.h"
#include "timer.h"
#include "chksum.h"
#include "pcconfig.h"
#include "pcqueue.h"
#include "pcstat.h"
#include "pcpkt.h"
#include "pcicmp.h"
#include "pctcp.h"
#include "pcdbug.h"
#include "netaddr.h"
#include "ip4_in.h"
#include "ip4_out.h"
#include "ip4_frag.h"

#define MAX_IP_FRAGS     2   /* max # of fragmented IP-packets */
#define MAX_IP_HOLDTIME  15  /* time (in sec) to hold before discarding */

int _ip4_frag_reasm = MAX_IP_HOLDTIME;

#if defined(USE_FRAGMENTS)

/*@-nullderef@*/

#if 0
#undef  MAX_FRAGMENTS
#define MAX_FRAGMENTS   20U   /* !! test */
#define MAX_FRAG_SIZE   (MAX_FRAGMENTS * MAX_IP4_DATA) // 29600
#endif

#define BUCKET_SIZE     (sizeof(HugeIP))  /* ~= 64k bytes */
#define BUCKET_MARKER   0xDEAFABBA

typedef struct huge_ip {
        in_Header hdr;
        BYTE      data [MAX_FRAG_SIZE];   /* 66600 for DOSX */
        DWORD     marker;
      } HugeIP;

typedef struct {
        DWORD  source;
        DWORD  destin;
        WORD   ident;
        BYTE   proto;
      } frag_key;

typedef struct {
        BOOL        used;     /* this bucket is taken */
        BOOL        got_ofs0; /* we've received ofs-0 fragment */
        int         active;   /* # of active fragments */
        frag_key    key;      /* key for matching new fragments */
        mac_address mac_src;  /* remember for icmp_send_timexceed() */
        HugeIP     *ip;       /* malloced, size = (BUCKET_SIZE) */
      } frag_bucket;

typedef struct hd {
        struct hd *next;
        long       start;
        long       end;
      } hole_descr;

typedef struct {
        BOOL        used;     /* this position in table in use */
        DWORD       timer;
        hole_descr *hole_first;
        in_Header  *ip;
        BYTE       *data_offset;
      } frag_hdr;


static frag_hdr    frag_list   [MAX_IP_FRAGS][MAX_FRAGMENTS];
static frag_bucket frag_buckets[MAX_IP_FRAGS];

static long        data_start;  /* NB! global data; not reentrant */
static long        data_end;
static long        data_length;
static BOOL        more_frags;

static BOOL       setup_first_frag  (const in_Header *ip, int idx);
static in_Header *alloc_frag_buffer (const in_Header *ip, int idx);

#if defined(__DJGPP__) && 0
  #define FIND_FRAG_BUG
#endif

#if defined(TEST_PROG) || defined(FIND_FRAG_BUG)
  #define MSG(x)  printf x
#else
  #define MSG(x)  ((void)0)
#endif


/*
 * Check if this packet is part of a fragment-chain. It might be
 * a last fragment (MF=0) with ofs 0. This would be normal for
 * fragments sent from Linux. Look in 'frag_buckets' for a match.
 *
 * If not found, return FALSE and set '*free' to free bucket.
 * If found, return TRUE and set '*slot' to free slot in frag-chain.
 */
static BOOL match_frag (const in_Header *ip, int *slot, int *bucket, int *free)
{
  frag_key key;
  int      i, j;

  *bucket = -1;
  *slot   = -1;
  *free   = -1;

  key.source = ip->source;
  key.destin = ip->destination;
  key.ident  = ip->identification;
  key.proto  = ip->proto;

  for (j = 0; j < MAX_IP_FRAGS; j++)
  {
    const frag_bucket *frag = &frag_buckets[j];

    if (!frag->used)
    {
      if (*free == -1)   /* get 1st vacant bucket */
         *free = j;
      continue;
    }

    if (memcmp(&key,&frag->key,sizeof(key)))
       continue;

    for (i = 0; i < (int)MAX_FRAGMENTS; i++)
    {
      if (!frag_list[j][i].used)
      {
        *bucket = j;
        return (TRUE);
      }
      *slot = i;
    }
  }
  return (FALSE);
}

/*
 * Check and report if fragment data-offset is okay
 */
static __inline int check_data_start (const in_Header *ip, DWORD ofs)
{
  if (ofs <= MAX_FRAG_SIZE && /* fragment offset okay, < 65528 */
      ofs <= USHRT_MAX-8)
     return (1);

  TCP_CONSOLE_MSG (2, (_LANG("Bad frag-ofs: %lu, ip-prot %u (%s -> %s)\n"),
                   ofs, ip->proto,
                   _inet_ntoa(NULL,intel(ip->source)),
                   _inet_ntoa(NULL,intel(ip->destination))));
  ARGSUSED (ip);
  return (0);
}

/*
 * ip4_defragment() is called if '*ip_ptr' is part of a new or excisting
 * fragment chain.
 *
 * IP header already checked in _ip4_handler().
 * Return the reassembled segment (ICMP, UDP or TCP) in '*ip_ptr'.
 * We assume MAC-header is the same on all fragments.
 * We return a packet with the MAC-header of the first
 * fragment received.
 */
int ip4_defragment (const in_Header **ip_ptr, DWORD offset, WORD flags)
{
  frag_hdr   *frag      = NULL;
  hole_descr *hole      = NULL;
  hole_descr *prev_hole = NULL;
  const in_Header  *ip  = *ip_ptr;

  BOOL  found    = FALSE;
  BOOL  got_hole = FALSE;
  int   i, j, j_free;

  /* Check if part of an existing frag-chain or a new fragment
   */
  if (!match_frag(ip,&i,&j,&j_free))
  {
    if (i == MAX_FRAGMENTS-1)   /* found but no slots free */
    {
      STAT (ip4stats.ips_fragments++);
      STAT (ip4stats.ips_fragdropped++);
      return (0);
    }

    if (offset == 0)
       return (1);

    /* Not in frag-list, but ofs > 0. Must be a part of a new
     * fragment chain.
     */
  }

  STAT (ip4stats.ips_fragments++);

  MSG (("\nip4_defrag: src %s, dst %s, id %04X, ofs %lu, flag %04X\n",
        _inet_ntoa(NULL,intel(ip->source)),
        _inet_ntoa(NULL,intel(ip->destination)),
        intel16(ip->identification), offset, flags));

  /* Calculate where data should go
   */
  data_start  = (long) offset;
  data_length = (long) intel16 (ip->length) - in_GetHdrLen (ip);
  data_end    = data_start + data_length;
  more_frags  = FALSE;  // !!! (flags & IP_MF);

  if (!check_data_start(ip,data_start))
  {
    STAT (ip4stats.ips_fragdropped++);
    DEBUG_RX (NULL, ip);
    return (0);
  }

  found = (j > -1);

  if (!found)
  {
    MSG (("bucket=%d, i=%d, key not found\n", j_free, i));
    j = j_free;

    /* Can't handle any new frags, biff packet
     */
    if (j == -1 || frag_buckets[j].active == MAX_FRAGMENTS)
    {
      STAT (ip4stats.ips_fragdropped++);
      MSG (("all buckets full\n"));
    }
    else
    {
      setup_first_frag (ip, j); /* Setup first fragment received */
      if (offset == 0)
         frag_buckets[j].got_ofs0 = TRUE;
    }
    DEBUG_RX (NULL, ip);
    return (0);
  }

  frag = &frag_list[j][i];

  MSG (("bucket=%d, slot=%d key found, more_frags=%d, active=%d\n",
        j, i, more_frags ? 1 : 0, frag_buckets[j].active));

#if 0
  if (!more_frags)           /* Adjust length  */
     frag->ip->length = intel16 ((WORD)(data_end + in_GetHdrLen(ip)));
#endif

  if (offset == 0)
     frag_buckets[j].got_ofs0 = TRUE;

  hole = frag->hole_first;   /* Hole handling */

  do
  {
    if (hole && (data_start <= hole->end) &&   /* We've found the spot */
        (data_end >= hole->start))
    {
      long temp = hole->end;    /* Pick up old hole end for later */

      got_hole = 1;

      more_frags = (intel16(frag->ip->frag_ofs) & IP_MF);  // !!

      /* Find where to insert fragment.
       * Check if there's a hole before the new frag
       */
      if (data_start > hole->start)
      {
        hole->end = data_start - 1;
        prev_hole = hole;  /* We have a new prev */
      }
      else
      {
        /* No, delete current hole
         */
        if (!prev_hole)
             frag->hole_first = hole->next;
        else prev_hole->next  = hole->next;
      }

      /* Is there a hole after the current fragment?
       * Only if we're not last and more to come
       */
      if (data_end < hole->end /* !! && more_frags */)
      {
        hole = (hole_descr*) (data_end + 1 + frag->data_offset);
        hole->start = data_end + 1;
        hole->end   = temp;

        /* prev_hole = NULL if first
         */
        if (!prev_hole)
        {
          hole->next = frag->hole_first;
          frag->hole_first = hole;
        }
        else
        {
          hole->next = prev_hole->next;
          prev_hole->next = hole;
        }
      }
    }
    prev_hole = hole;
    hole = hole->next;
  }
  while (hole);          /* Until we got to the end or found */


  /* Thats all setup so copy in the data
   */
  if (got_hole)
     memcpy (frag->data_offset + data_start,
             (BYTE*)ip + in_GetHdrLen(ip), (size_t)data_length);

  MSG (("got_hole %d, frag->hole_first %lX\n",
        got_hole, (DWORD)frag->hole_first));

  if (!frag->hole_first /* !! && !more_frags */)  /* Now we have all the parts */
  {
    if (frag_buckets[j].active >= 1)
        frag_buckets[j].active--;

    /* Redo checksum as we've changed the length in the header
     */
    frag->ip->frag_ofs = 0;    /* no MF or frag-ofs */
    frag->ip->checksum = 0;
    frag->ip->checksum = ~CHECKSUM (frag->ip, sizeof(in_Header));

    STAT (ip4stats.ips_reassembled++);
    *ip_ptr = frag->ip;      /* MAC-header is in front of IP */
    return (1);
  }
  DEBUG_RX (NULL, ip);
  ARGSUSED (flags);
  return (0);
}

/*
 * Prepare and setup for reassembly
 */
static BOOL setup_first_frag (const in_Header *ip, int idx)
{
  frag_hdr   *frag;
  in_Header  *bucket;
  hole_descr *hole;
  unsigned    i;

  /* Allocate a fragment bucket. MAC-header is in front of bucket.
   */
  bucket = alloc_frag_buffer (ip, idx);
  if (!bucket)
  {
    STAT (ip4stats.ips_fragdropped++);
    MSG (("alloc bucket failed\n"));
    return (FALSE);
  }

  /* Find first empty slot
   */
  frag = &frag_list[idx][0];
  for (i = 0; i < MAX_FRAGMENTS; i++, frag++)
      if (!frag->used)
         break;

  if (i == MAX_FRAGMENTS)
  {
    STAT (ip4stats.ips_fragdropped++);
    MSG (("frag_list[%d] full\n", idx));
    return (FALSE);
  }

  frag->used = TRUE;            /* mark as used */
  frag_buckets[idx].active++;   /* inc active frags counter */

  MSG (("bucket=%d, active=%u, i=%u\n", idx, frag_buckets[idx].active, i));

  /* Remember MAC source address
   */
  if (_pktserial)
       memset (&frag_buckets[idx].mac_src, 0, sizeof(mac_address));
  else memcpy (&frag_buckets[idx].mac_src, MAC_SRC(ip), sizeof(mac_address));

  /* Setup frag header data, first packet
   */
  frag_buckets[idx].key.proto  = ip->proto;
  frag_buckets[idx].key.source = ip->source;
  frag_buckets[idx].key.destin = ip->destination;
  frag_buckets[idx].key.ident  = ip->identification;

  frag->ip    = bucket;
  frag->timer = set_timeout (1000 * min(_ip4_frag_reasm, ip->ttl));

  /* Set pointers to beginning of IP packet data
   */
  frag->data_offset = (BYTE*)bucket + in_GetHdrLen(ip);

  /* Setup initial hole table
   */
  if (data_start == 0)  /* 1st fragment sent is 1st fragment received */
  {
    WORD  ip_len = intel16 (ip->length);
    BYTE *dst    = (BYTE*)bucket;

    memcpy (dst, ip, min(ip_len,_mtu));
    hole = (hole_descr*) (dst + ip_len + 1);
    frag->hole_first = hole;
  }
  else
  {
    /* !!fix-me: assumes header length of this fragment is same as
     *           in reassembled IP packet (may have IP-options)
     */
    BYTE *dst = frag->data_offset + data_start;
    BYTE *src = (BYTE*)ip + in_GetHdrLen(ip);

    memcpy (dst, src, (size_t)data_length);

    /* Bracket beginning of data
     */
    hole        = frag->hole_first = (hole_descr*)frag->data_offset;
    hole->start = 0;
    hole->end   = data_start - 1;
  //if (more_frags)
    {
      hole->next = (hole_descr*) (frag->data_offset + data_length + 1);
      hole = hole->next;
    }
#if 0
    else
    {
      hole = frag->hole_first->next = NULL;
      /* Adjust length */
      frag->ip->length = intel16 ((WORD)(data_end + in_GetHdrLen(ip)));
    }
#endif
  }

  if (hole)
  {
    hole->start = data_length;
    hole->end   = MAX_FRAG_SIZE;
    hole->next  = NULL;

    MSG (("hole %lX, start %lu, end %lu\n",
          (DWORD)hole, hole->start, hole->end));
  }
  return (TRUE);
}


/*
 * Allocate a bucket for doing fragment reassembly
 */
static in_Header *alloc_frag_buffer (const in_Header *ip, int j)
{
  BYTE *p;

  if (!frag_buckets[j].ip)
  {
    p = (BYTE*) calloc (BUCKET_SIZE + _pkt_ip_ofs, 1);
    if (!p)
       return (NULL);

    frag_buckets[j].ip = (HugeIP*) (p + _pkt_ip_ofs);
    ((HugeIP*)p)->marker = BUCKET_MARKER;
  }
  else
  {
    p = (BYTE*)frag_buckets[j].ip - _pkt_ip_ofs;
    if (((HugeIP*)p)->marker != BUCKET_MARKER)
    {
      TCP_CONSOLE_MSG (0, ("frag_buckets[%d] destroyed\n!", j));
      free (p);
      frag_buckets[j].ip   = NULL;
      frag_buckets[j].used = FALSE;
      return (NULL);
    }
  }
  MSG (("alloc_frag() = %lX\n", (DWORD)frag_buckets[j].ip));

  frag_buckets[j].used     = TRUE;
  frag_buckets[j].got_ofs0 = FALSE;

  if (!_pktserial)
     memcpy (p, MAC_HDR(ip), _pkt_ip_ofs);  /* remember MAC-head */
  return (&frag_buckets[j].ip->hdr);
}

/*
 * Free/release the reassembled IP-packet.
 * Note: we don't free it to the heap.
 */
int ip4_free_fragment (const in_Header *ip)
{
  unsigned i, j;

  for (j = 0; j < MAX_IP_FRAGS; j++)
      if (ip == &frag_buckets[j].ip->hdr && frag_buckets[j].used)
      {
        MSG (("ip4_free_fragment(%lX), bucket=%d, active=%d\n",
              (DWORD)ip, j, frag_buckets[j].active));

        frag_buckets[j].used   = FALSE;
        frag_buckets[j].active = 0;
        for (i = 0; i < MAX_FRAGMENTS; i++)
            frag_list[j][i].used = FALSE;
        return (1);
      }
  return (0);
}

/*
 * Check if any fragment chains has timed-out
 */
void chk_timeout_frags (void)
{
  unsigned i, j;

  for (j = 0; j < MAX_IP_FRAGS; j++)
  {
    for (i = 0; i < MAX_FRAGMENTS; i++)
    {
      in_Header ip;

      if (!frag_buckets[j].active ||
          !frag_list[j][i].used   ||
          !chk_timeout(frag_list[j][i].timer))
         continue;

      memset (&ip, 0, sizeof(ip));
      ip.identification = frag_buckets[j].key.ident;
      ip.proto          = frag_buckets[j].key.proto;
      ip.source         = frag_buckets[j].key.source;
      ip.destination    = frag_buckets[j].key.destin;

      if (frag_buckets[j].got_ofs0)
      {
        if (_pktserial)    /* send an ICMP_TIMXCEED (code 1) */
             icmp_send_timexceed (&ip, NULL);
        else icmp_send_timexceed (&ip, (const void*)&frag_buckets[j].mac_src);
      }

      STAT (ip4stats.ips_fragtimeout++);

      MSG (("chk_timeout_frags(), bucket %u, slot %d, id %04X\n",
            j, i, intel16(ip.identification)));

      if (ip4_free_fragment(&frag_buckets[j].ip->hdr))
         return;
    }
  }
}

/*----------------------------------------------------------------------*/

#if defined(TEST_PROG)  /* a small test program (djgpp/Watcom/HighC) */
#undef FP_OFF
#undef enable
#undef disable

#include <unistd.h>

#include "sock_ini.h"
#include "loopback.h"
#include "pcarp.h"
#include "getopt.h"

static DWORD to_host   = 0;
static WORD  frag_ofs  = 0;
static int   max_frags = 5;
static int   frag_size = 1000;
static int   rand_frag = 0;
static int   rev_order = 0;
static int   time_frag = 0;

void usage (char *argv0)
{
  printf ("%s [-n num] [-s size] [-h ip] [-r] [-R] [-t]\n"
          "Send fragmented ICMP Echo Request (ping)\n\n"
          "options:\n"
          "  -n  number of fragments to send     (default %d)\n"
          "  -s  size of each fragment           (default %d)\n"
          "  -h  specify destination IP          (default 127.0.0.1)\n"
          "  -r  send fragments in random order  (default %s)\n"
          "  -R  send fragments in reverse order (default %s)\n"
          "  -t  simulate fragment timeout       (default %s)\n",
          argv0, max_frags, frag_size,
          rand_frag ? "yes" : "no",
          rev_order ? "yes" : "no",
          time_frag ? "yes" : "no");
  exit (0);
}

BYTE *init_frag (int argc, char **argv)
{
  int   i, ch;
  BYTE *data;

  while ((ch = getopt(argc, argv, "h:n:s:rRt?")) != EOF)
     switch (ch)
     {
       case 'h': to_host = inet_addr (optarg);
                 if (to_host == INADDR_NONE)
                 {
                   printf ("Illegal IP-address\n");
                   exit (-1);
                 }
                 break;
       case 'n': max_frags = atoi (optarg);
                 break;
       case 's': frag_size = atoi (optarg) / 8;  /* multiples of 8 */
                 frag_size <<= 3;
                 break;
       case 'r': rand_frag = 1;
                 break;
       case 'R': rev_order = 1;
                 break;
       case 't': time_frag = 1;   /** \todo Simulate fragment timeout */
                 break;
       case '?':
       default : usage (argv[0]);
     }

  if (max_frags < 1 || max_frags > FD_SETSIZE)
  {
    printf ("# of fragments is 1 - %d\n", FD_SETSIZE);
    exit (-1);
  }

  if (frag_size < 8 || frag_size > MAX_IP4_DATA)
  {
    printf ("Fragsize range is 8 - %ld\n", MAX_IP4_DATA);
    exit (-1);
  }

  if (frag_size * max_frags > USHRT_MAX)
  {
    printf ("Total fragsize > 64kB!\n");
    exit (-1);
  }

  data = calloc (frag_size * max_frags, 1);
  if (!data)
  {
    printf ("no memory\n");
    exit (-1);
  }

  for (i = 0; i < max_frags; i++)
     memset (data + i*frag_size, 'a'+i, frag_size);

  loopback_mode |= LBACK_MODE_ENABLE;
  dbug_init();
  sock_init();

  if (!to_host)
     to_host = htonl (INADDR_LOOPBACK);
  return (data);
}

/*----------------------------------------------------------------------*/

int rand_packet (fd_set *fd, int max)
{
  int count = 0;

  while (1)
  {
    int i = Random (0, max);
    if (i < max && !FD_ISSET(i,fd))
    {
      FD_SET (i, fd);
      return (i);
    }
    if (++count == 10*max)
       return (-1);
  }
}

/*----------------------------------------------------------------------*/

int main (int argc, char **argv)
{
  fd_set      is_sent;
  int         i;
  eth_address eth;
  in_Header  *ip;
  ICMP_PKT   *icmp;
  WORD        frag_flag;
  BYTE       *data = init_frag (argc, argv);

  if (!_arp_resolve (ntohl(to_host), &eth))
  {
    printf ("ARP failed\n");
    return (-1);
  }

  ip   = (in_Header*) _eth_formatpacket (&eth, IP4_TYPE);
  icmp = (ICMP_PKT*) data;

  ip->hdrlen         = sizeof(*ip)/4;
  ip->ver            = 4;
  ip->tos            = _default_tos;
  ip->identification = _get_ip4_id();
  ip->ttl            = 15;           /* max 15sec reassembly time */
  ip->proto          = ICMP_PROTO;

  icmp->echo.type       = ICMP_ECHO;
  icmp->echo.code       = 0;
  icmp->echo.identifier = 0;
  icmp->echo.index      = 1;
  icmp->echo.sequence   = set_timeout (1);  /* "random" id */
  icmp->echo.checksum   = 0;
  icmp->echo.checksum   = ~CHECKSUM (icmp, max_frags*frag_size);

  FD_ZERO (&is_sent);

#if 0  /* test random generation */
  if (rand_frag)
     for (i = 0; i < max_frags; i++)
     {
       int j = rand_packet (&is_sent, max_frags);
       printf ("index %d\n", j);
     }
  exit (0);
#endif

  for (i = 0; i < max_frags; i++)
  {
    int j;

    if (rand_frag)
    {
      j = rand_packet (&is_sent, max_frags);
      if (j < 0)
         break;
    }
    if (rev_order)
    {
      j = max_frags - i - 1;
      frag_flag = (j > 0) ? IP_MF : 0;
    }
    else
    {
      j = i;
      frag_flag = (j < max_frags) ? IP_MF : 0;
    }

    frag_ofs = (j * frag_size);
    memcpy ((BYTE*)(ip+1), data+frag_ofs, frag_size);

    /* The loopback device swaps src/dest IP; hence we must set them
     * on each iteration of the loop
     */
    ip->source      = intel (my_ip_addr);
    ip->destination = to_host;
    ip->frag_ofs    = intel16 (frag_ofs/8 | frag_flag);
    ip->length      = intel16 (frag_size + sizeof(*ip));
    ip->checksum    = 0;
    ip->checksum    = ~CHECKSUM (ip, sizeof(*ip));

    _eth_send (frag_size+sizeof(*ip), NULL, __FILE__, __LINE__);

    STAT (ip4stats.ips_ofragments++);
    tcp_tick (NULL);
  }

  puts ("tcp_tick()");

  /* poll the IP-queue, reassemble fragments and
   * dump reassembled IP/ICMP packet to debugger (look in wattcp.dbg)
   */
  if (to_host != htonl (INADDR_LOOPBACK))
  {
#if defined(__HIGHC__) || defined(__WATCOMC__)
    sleep (1);
#else
    usleep (1000000 + 100*max_frags*frag_size); /* wait for reply */
#endif
  }

  tcp_tick (NULL);
  return (0);
}
#endif  /* TEST_PROG */
#endif  /* USE_FRAGMENTS */

