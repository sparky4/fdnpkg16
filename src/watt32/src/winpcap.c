/*!\file winpcap.c
 *
 *  Packet-driver like interface to WinPcap.
 *
 *  Copyright (c) 2004 Gisle Vanem <giva@bgnett.no>
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
 */

#include "wattcp.h"

#if defined(WIN32)

#include <windowsx.h>
#include <process.h>
#include <arpa/inet.h>

#include "strings.h"
#include "misc.h"
#include "timer.h"
#include "sock_ini.h"
#include "language.h"
#include "pcconfig.h"
#include "pcdbug.h"
#include "pcstat.h"
#include "pcpkt.h"

#include "winpcap.h"
#include "ntddndis.h"
#include "packet32.h"

#if (defined(_DLL) && !defined(_MT)) && !defined(__LCC__)
#error This file must be compiled for threads
#endif

#ifndef OID_GEN_PHYSICAL_MEDIUM
#define OID_GEN_PHYSICAL_MEDIUM  0x00010202
#endif

#ifndef OID_FDDI_LONG_PERMANENT_ADDRESS
#define OID_FDDI_LONG_PERMANENT_ADDRESS  0x03010101
#endif

#ifndef NDIS_FLAGS_SKIP_LOOPBACK_W2K
#define NDIS_FLAGS_SKIP_LOOPBACK_W2K  0x400
#endif

/* Rewritten from <NtDDNdis.h>
 */
#define ND_PHY_Unspecified   0
#define ND_PHY_WirelessLan   1
#define ND_PHY_CableModem    2
#define ND_PHY_PhoneLine     3
#define ND_PHY_PowerLine     4
#define ND_PHY_DSL           5
#define ND_PHY_FibreChannel  6
#define ND_PHY_1394          7
#define ND_PHY_WirelessWan   8

WORD _pktdevclass = PDCLASS_UNKNOWN;
WORD _pkt_ip_ofs  = 0;
BOOL _pktserial   = FALSE;
int  _pkt_errno   = 0;
int  _pkt_rxmode  =  RXMODE_DEFAULT;  /**< active receive mode */
int  _pkt_rxmode0 = -1;               /**< startup receive mode */
int  _pkt_forced_rxmode = -1;         /**< wattcp.cfg Rx mode */
char _pktdrvrname   [MAX_VALUELEN+1] = "";
char _pktdrvr_descr [MAX_VALUELEN+1] = "";

struct pkt_info *_pkt_inf = NULL;

/* Determines size of work buffers of NPF kernel and PacketReceivePacket().
 * It doesn't influence the size of thread queue yet.
 */
static int pkt_num_rx_bufs = RX_BUFS;

/* NPF doesn't keep a count of transmitted packets and errors.
 * So we maintain these counters locally.
 */
static DWORD num_tx_pkt    = 0UL;
static DWORD num_tx_bytes  = 0UL;
static DWORD num_tx_errors = 0UL;
static DWORD num_rx_bytes  = 0UL;
static DWORD pkt_drop_cnt  = 0UL;
static int   pkt_txretries = 2;
static int   thr_realtime  = 0;
static BOOL  thr_stopped   = FALSE;
static FILE *dump_file     = NULL;
static char  dump_fname [MAX_PATHLEN] = "$(TEMP)\\winpcap_dump.txt";

static const struct search_list logical_media[] = {
     { -1,                   "Unknown" },
     { NdisMedium802_3,      "802.3" },
     { NdisMedium802_5,      "802.5" },
     { NdisMediumFddi,       "FDDI" },
     { NdisMediumWan,        "WAN" },
     { NdisMediumLocalTalk,  "LocalTalk" },
     { NdisMediumDix,        "DIX" },
     { NdisMediumArcnetRaw,  "ArcNet (raw)" },
     { NdisMediumArcnet878_2,"ArcNet (878.2)" },
     { NdisMediumAtm,        "ATM" },
     { NdisMediumWirelessWan,"WiFi" },
     { NdisMediumIrda,       "IrDA" },
     { NdisMediumNull,       "Null?!" },
     { NdisMediumCHDLC,      "CHDLC" },
     { NdisMediumPPPSerial,  "PPP-ser" }
    };

static const struct search_list phys_media[] = {
     { ND_PHY_WirelessLan,  "Wireless LAN" },
     { ND_PHY_CableModem,   "Cable"        },
     { ND_PHY_PhoneLine,    "Phone"        },
     { ND_PHY_PowerLine,    "PowerLine"    },
     { ND_PHY_DSL,          "DSL"          },
     { ND_PHY_FibreChannel, "Fibre"        },
     { ND_PHY_1394,         "IEEE1394"     },
     { ND_PHY_WirelessWan,  "Wireless WAN" }
   };

static BOOL get_perm_mac_address (void *mac);
static BOOL get_interface_mtu (DWORD *mtu);
static BOOL get_interface_type (WORD *link);
static BOOL get_connected_status (BOOL *is_up);
static void show_link_details (void);
static void set_txmode (const char *value);

DWORD __stdcall pkt_recv_thread (void *arg);

/**
 * WATTCP.CFG parser for "PCAP." keywords in WATTCP.CFG.
 */
static BOOL parse_config_pass_1 (void)
{
  static const struct config_table pkt_init_cfg[] = {
                   { "PCAP.DEVICE",   ARG_STRCPY, (void*)&_pktdrvrname },
                   { "PCAP.DUMPFILE", ARG_STRCPY, (void*)&dump_fname },
                   { "PCAP.TXRETRIES",ARG_ATOB,   (void*)&pkt_txretries },
                   { "PCAP.TXMODE",   ARG_FUNC,   (void*)&set_txmode },
                   { "PCAP.RXMODE",   ARG_ATOX_W, (void*)&_pkt_forced_rxmode },
                   { "PCAP.RXBUFS",   ARG_ATOI,   (void*)&pkt_num_rx_bufs },
                   { "PCAP.HIGHPRIO", ARG_ATOI,   (void*)&thr_realtime },
                   { NULL,            0,          NULL }
                 };
  const struct config_table *cfg_save          = watt_init_cfg;
  void (*init_save) (const char*, const char*) = usr_init;
  int    rc;

  watt_init_cfg = pkt_init_cfg;
  usr_init      = NULL;    /* only pkt_init_cfg[] gets parsed */
  rc = tcp_config (NULL);
  usr_init      = init_save;
  watt_init_cfg = cfg_save;
  return (rc > 0);
}

/**
 * Traverse 'adapters_list' and select 1st device with an IPv4
 * address. Assuming that device is a physical adapter we can use.
 */
static BOOL find_adapter (char *aname, size_t size)
{
#if defined(USE_DYN_PACKET)
  ARGSUSED (aname);
  ARGSUSED (size);
  return (TRUE);   /**\todo Use IPhlpAPI function to return list of adapters */
#else
  const ADAPTER_INFO *ai;
  int   i;

  for (ai = PacketGetAdInfo(); ai; ai = ai->Next)
  {
    for (i = 0; i < ai->NNetworkAddresses; i++)
    {
      const npf_if_addr        *if_addr = ai->NetworkAddresses + i;
      const struct sockaddr_in *ip_addr = (const struct sockaddr_in*) &if_addr->IPAddress;

      if (ip_addr->sin_family == AF_INET)
      {
        StrLcpy (aname, ai->Name, size);
        return (TRUE);
      }
    }
  }
  return (FALSE);
#endif
}

/**
 * Initialise WinPcap and return our MAC address.
 */
int pkt_eth_init (mac_address *mac_addr)
{
  struct {
    PACKET_OID_DATA oidData;
    char            descr[512];
  } oid;
  const ADAPTER *adapter = NULL;
  DWORD thread_id;
  BOOL  is_up;

  if (_watt_is_win9x)  /**< \todo Support Win-9x too */
  {
    (*_printf) (_LANG("Win-NT or later reqired.\n"));
    _pkt_errno = PDERR_GEN_FAIL;
    return (WERR_ILL_DOSX);
  }

  if (!_watt_no_config || _watt_user_config_fn)
     parse_config_pass_1();

  _pkt_inf = calloc (sizeof(*_pkt_inf), 1);
  if (!_pkt_inf)
  {
    (*_printf) (_LANG("Failed to allocate WinPcap DRIVER data.\n"));
    _pkt_errno = PDERR_GEN_FAIL;
    return (WERR_NO_MEM);
  }

  if (debug_on >= 2 && dump_fname[0])
     dump_file = fopen_excl (ExpandVarStr(dump_fname), "w+t");

  if (!PacketInitModule(TRUE, dump_file))
  {
    (*_printf) (_LANG("Failed to initialise WinPcap.\n"));
    pkt_release();
    _pkt_errno = PDERR_NO_DRIVER;
    return (WERR_PKT_ERROR);
  }

  if (!_pktdrvrname[0] &&
      !find_adapter(_pktdrvrname,sizeof(_pktdrvrname)))
  {
    (*_printf) (_LANG("No WinPcap driver found.\n"));
    _pkt_errno = PDERR_NO_DRIVER;
    return (WERR_NO_DRIVER);
  }

  TCP_CONSOLE_MSG (2, ("device %s\n", _pktdrvrname));

  adapter = PacketOpenAdapter (_pktdrvrname);
  if (!adapter)
  {
    if (debug_on > 0)
       (*_printf) (_LANG("PacketOpenAdapter (\"%s\") failed; %s\n"),
                   _pktdrvrname, win_strerror(GetLastError()));
    pkt_release();
    return (WERR_NO_DRIVER);
  }

  _pkt_inf->adapter = adapter;
#if defined(USE_DYN_PACKET)
  _pkt_inf->adapter_info = NULL;
#else
  _pkt_inf->adapter_info = PacketFindAdInfo (_pktdrvrname);
#endif

  /* Query the NIC driver for the adapter description
   */
  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = OID_GEN_VENDOR_DESCRIPTION;
  oid.oidData.Length = sizeof(oid.descr);

  if (PacketRequest (adapter, FALSE, &oid.oidData))
     StrLcpy (_pktdrvr_descr, (char*)oid.oidData.Data, sizeof(_pktdrvr_descr));
  else
  {
    (*_printf) (_LANG("PacketRequest() failed; %s\n"),
                win_strerror(GetLastError()));
    pkt_release();
    return (WERR_PKT_ERROR);
  }

  if (!get_interface_type(&_pktdevclass))
  {
    pkt_release();
    return (WERR_PKT_ERROR);
  }

  if (get_connected_status(&is_up) && !is_up)
     (*_printf) (_LANG("Warning: the adapter %s is down\n"), _pktdrvrname);

  switch (_pktdevclass)
  {
    case PDCLASS_TOKEN:
         _pkt_ip_ofs = sizeof(tok_Header);
         break;
    case PDCLASS_ETHER:
         _pkt_ip_ofs = sizeof(eth_Header);
         break;
    case PDCLASS_FDDI:
         _pkt_ip_ofs = sizeof(fddi_Header);
         break;
    case PDCLASS_ARCNET:
         _pkt_ip_ofs = ARC_HDRLEN;
         break;
    default:
         pkt_release();
         (*_printf) (_LANG("WinPcap-ERROR: Unsupported driver class %dh\n"),
                     _pktdevclass);
         _pkt_errno = PDERR_NO_CLASS;
         return (WERR_PKT_ERROR);
  }

  if (!pkt_get_addr(mac_addr))  /* get our MAC address */
  {
    pkt_release();
    return (WERR_PKT_ERROR);
  }

  pktq_init (&_pkt_inf->pkt_queue,
             sizeof(_pkt_inf->rx_buf[0]),   /* RX_SIZE */
             DIM(_pkt_inf->rx_buf),         /* RX_BUFS */
             (char*)&_pkt_inf->rx_buf);

  _pkt_inf->npf_buf_size = RX_SIZE * pkt_num_rx_bufs;
  _pkt_inf->npf_buf = malloc (_pkt_inf->npf_buf_size);
  if (!_pkt_inf->npf_buf)
  {
    (*_printf) (_LANG("Failed to allocate %d byte Rx buffer.\n"),
                _pkt_inf->npf_buf_size);
    pkt_release();
    _pkt_errno = PDERR_GEN_FAIL;
    return (WERR_NO_MEM);
  }

  PacketSetMode (adapter, PACKET_MODE_CAPT);
  PacketSetBuff (adapter, _pkt_inf->npf_buf_size);
  PacketSetMinToCopy (adapter, ETH_MIN);

  /* PacketReceivePacket() blocks until something is received
   */
  PacketSetReadTimeout ((ADAPTER*)adapter, 0);

  /* Set Rx-mode forced via config.
   */
  if (_pkt_forced_rxmode != -1)
  {
    _pkt_forced_rxmode &= 0xFFFF;     /* clear bits not set via ARG_ATOX_W */
    if (_pkt_forced_rxmode == 0 ||    /* check illegal bit-values */
        (_pkt_forced_rxmode & 0x10) ||
        (_pkt_forced_rxmode & 0x40) ||
        (_pkt_forced_rxmode > 0x80))
     {
       TCP_CONSOLE_MSG (0, ("Illegal Rx-mode (0x%02X) specified\n",
                        _pkt_forced_rxmode));
       _pkt_forced_rxmode = -1;
     }
  }

  if (pkt_get_rcv_mode())
     _pkt_rxmode0 = _pkt_rxmode;

  if (_pkt_forced_rxmode != -1)
       pkt_set_rcv_mode (_pkt_forced_rxmode);
  else pkt_set_rcv_mode (RXMODE_DEFAULT);

#if 1
  _pkt_inf->recv_thread = CreateThread (NULL, 2048, pkt_recv_thread,
                                        NULL, 0, &thread_id);
#else
  _pkt_inf->recv_thread = _beginthreadex (NULL, 2048, pkt_recv_thread,
                                          NULL, 0, &thread_id);
#endif

  if (!_pkt_inf->recv_thread)
  {
    (*_printf) (_LANG("Failed to create receiver thread; %s\n"),
                win_strerror(GetLastError()));
    pkt_release();
    _pkt_errno = PDERR_GEN_FAIL;
    return (WERR_PKT_ERROR);
  }

  if (thr_realtime)
     SetThreadPriority (_pkt_inf->recv_thread,
                        THREAD_PRIORITY_TIME_CRITICAL);

  TCP_CONSOLE_MSG (2, ("capture thread-id %lu\n", thread_id));

#if defined(USE_DEBUG)
  if (debug_on >= 2)
  {
    (*_printf) ("link-details:\n");
    show_link_details();
  }
#endif
  return (0);
}

int pkt_release (void)
{
  ADAPTER *adapter;
  DWORD    status = 1;

  if (!_pkt_inf || _watt_fatal_error)
     return (0);

  adapter = (ADAPTER*) _pkt_inf->adapter;

  TCP_CONSOLE_MSG (2, ("pkt_release(): adapter %08lX\n", (DWORD)adapter));

  if (adapter && adapter != INVALID_HANDLE_VALUE)
  {
    /* Don't close the adapter before telling the thread about it.
     */
    if (_pkt_inf->recv_thread)
    {
      FILETIME ctime, etime, ktime, utime;
      DWORD    err = 0;

      _pkt_inf->stop_thread = TRUE;
      SetEvent (adapter->ReadEvent);
      Sleep (50);

      GetExitCodeThread (_pkt_inf->recv_thread, &status);
      if (status == STILL_ACTIVE)
           TCP_CONSOLE_MSG (2, ("pkt_release(): killing thread.\n"));
      else TCP_CONSOLE_MSG (2, ("pkt_release(): thread exit code %lu.\n",
                            status));

      if (!GetThreadTimes (_pkt_inf->recv_thread, &ctime, &etime, &ktime, &utime))
         err = GetLastError();
      TerminateThread (_pkt_inf->recv_thread, 1);
      CloseHandle (_pkt_inf->recv_thread);

      if (err)
         TCP_CONSOLE_MSG (2, ("  GetThreadTime() %s, ", win_strerror(err)));
      TCP_CONSOLE_MSG (2, ("  kernel time %.6fs, user time %.6fs\n",
                       filetime_sec(&ktime), filetime_sec(&utime)));
    }

    PacketCloseAdapter (adapter);

    _pkt_inf->adapter = NULL;
  }

  DO_FREE (_pkt_inf->npf_buf);
  DO_FREE (_pkt_inf);

  PacketInitModule (FALSE, dump_file);
  if (dump_file)
     fclose (dump_file);
  dump_file = NULL;
  return (int) status;
}

/*
 * Return the MAC address.
 */
int pkt_get_addr (mac_address *eth)
{
  mac_address mac;

  if (get_perm_mac_address(&mac))
  {
    memcpy (eth, &mac, sizeof(*eth));
    return (1);
  }
  TCP_CONSOLE_MSG (2, ("Failed to get our MAC address; %s\n",
                   win_strerror(GetLastError())));
  return (0);
}

int pkt_set_addr (const void *eth)
{
  ARGSUSED (eth);
  return (0);
}

/*
 * Check 'my_ip' against the IPv4 address(es) Winsock is using for
 * this adapter.
 */
BOOL pkt_check_address (DWORD my_ip)
{
#if !defined(USE_DYN_PACKET)
  const ADAPTER_INFO *ai;
  int   i;

  if (!_pkt_inf)
     return (TRUE);

  ai = (const ADAPTER_INFO*) _pkt_inf->adapter_info;

  for (i = 0; i < ai->NNetworkAddresses; i++)
  {
    const npf_if_addr        *if_addr = ai->NetworkAddresses + i;
    const struct sockaddr_in *ip_addr = (const struct sockaddr_in*) &if_addr->IPAddress;

    if (ip_addr->sin_addr.s_addr == htonl(my_ip))
       return (FALSE);
  }
#else
  ARGSUSED (my_ip);
#endif
  return (TRUE);  /* Okay, no possible conflict */
}


int pkt_buf_wipe (void)
{
  if (_pkt_inf)
     pktq_clear (&_pkt_inf->pkt_queue);
  return (1);
}

int pkt_waiting (void)
{
  if (_pkt_inf)
     return pktq_queued (&_pkt_inf->pkt_queue);
  return (-1);
}

void pkt_free_pkt (const void *pkt)
{
  struct pkt_ringbuf *q;
  const  char *q_tail;
  int    delta;

  if (!_pkt_inf || !pkt)
     return;

  q = &_pkt_inf->pkt_queue;
  pkt_drop_cnt = q->num_drop;

  q_tail = (const char*)pkt - _pkt_ip_ofs - RX_ELEMENT_HEAD_SIZE;
  delta  = q_tail - pktq_out_buf (q);
  if (delta)
  {
    TCP_CONSOLE_MSG (0, ("%s: freeing illegal packet 0x%p, delta %d.\n",
                     __FILE__, pkt, delta));
    pktq_clear (q);
  }
  else
    pktq_inc_out (q);
}

/**
 * Select async (overlapping) tx-mode. NOT YET.
 */
static void set_txmode (const char *value)
{
  ARGSUSED (value);
}

/**
 * Our low-level capture thread.
 *
 * Loop waiting for packet(s) in PacketReceivePacket(). In
 * _pkt_release() the receive event-handle (adapter->Readvent) is
 * set. This causes WaitForSingleObject() in PacketReceivePacket()
 * to return. It is vital that the ADAPTER object isn't deallocated
 * before PacketReceivePacket() returns.
 *
 * The return-value is > 0 if thread exited on it's own. Otherwise
 * it is 0  (from  GetExitCodeThread).
 */
DWORD __stdcall pkt_recv_thread (void *arg)
{
  int rc = 0;

  while (1)
  {
    const  ADAPTER *adapter;
    const  BYTE *pkt, *pkt_end;
    size_t cap_len, hdr_len;
    int    total_len, chunk;
    PACKET npf_pkt;

    if (!_pkt_inf || _pkt_inf->stop_thread)  /* signals closure */
    {
      rc = 1;
      break;
    }

    adapter  = (const ADAPTER*) _pkt_inf->adapter;
    npf_pkt.Length = _pkt_inf->npf_buf_size;
    npf_pkt.Buffer = _pkt_inf->npf_buf;

    if (!PacketReceivePacket(adapter, &npf_pkt, TRUE))
    {
      rc = 2;
      break;
    }
    total_len = npf_pkt.ulBytesReceived;

    ENTER_CRIT();

    for (pkt = npf_pkt.Buffer, pkt_end = (BYTE*)npf_pkt.Buffer + total_len, chunk = 1;
         pkt < pkt_end;
         pkt += Packet_WORDALIGN(cap_len+hdr_len), chunk++)
    {
      struct pkt_ringbuf    *q;
      struct pkt_rx_element *head;
      struct bpf_hdr        *bp;

      q  = &_pkt_inf->pkt_queue;
      bp = (struct bpf_hdr*) pkt;

      cap_len = bp->bh_caplen;
      hdr_len = bp->bh_hdrlen;

      num_rx_bytes += bp->bh_datalen;

      TCP_CONSOLE_MSG (2, ("pkt_recv_thread(): total_len %d, "
                           "chunk %d, cap_len %d, in-idx %d\n",
                       total_len, chunk, cap_len, q->in_index));

      if (cap_len > ETH_MAX)
      {
        _pkt_inf->error = "Large size";
        STAT (macstats.num_too_large++);
      }
      else
      if (pktq_in_index(q) == q->out_index)  /* queue is full, drop it */
         q->num_drop++;
      else
      {
        int pad_len, buf_max;

        head = (struct pkt_rx_element*) pktq_in_buf (q);
        memcpy (head->rx_buf, pkt + hdr_len, cap_len);
        head->tstamp_put = bp->bh_tstamp;
        head->rx_length  = cap_len;

        /* zero-pad up to ETH_MAX (don't destroy marker at end)
         */
        buf_max = q->buf_size - RX_ELEMENT_HEAD_SIZE - 4;
        pad_len = min (ETH_MAX, buf_max - cap_len);
        if (pad_len > 0)
           memset (&head->rx_buf[cap_len], 0, pad_len);
        pktq_inc_in (q);
      }
    }
    LEAVE_CRIT();
  }

  TCP_CONSOLE_MSG (2, ("pkt_recv_thread(): rc %d\n", rc));
  fflush (stdout);
  ARGSUSED (arg);
  thr_stopped = TRUE;
  return (rc);
}

/*
 * Poll the tail of queue
 */
struct pkt_rx_element *pkt_poll_recv (void)
{
  struct pkt_ringbuf    *q;
  struct pkt_rx_element *rc = NULL;
  const char  *out;
  BOOL  empty;

  if (!_pkt_inf)
     return (NULL);

  ENTER_CRIT();
  q = &_pkt_inf->pkt_queue;
  out = pktq_out_buf (q);
  empty = (q->in_index == q->out_index);
  LEAVE_CRIT();

  if (!empty)
  {
    rc = (struct pkt_rx_element*) out;
    gettimeofday (&rc->tstamp_get, NULL);
  }
  return (rc);
}

/*
 * Our WinPcap send function.
 * Not sure if partial writes are possible with NPF. Retry if rc != length.
 */
int pkt_send (const void *tx, int length)
{
  const ADAPTER *adapter;
  PACKET pkt;
  int    tx_cnt, rc = 0;

  ASSERT_PKT_INF (0);

  adapter = (const ADAPTER*) _pkt_inf->adapter;

  PROFILE_START ("pkt_send");

  for (tx_cnt = 1 + pkt_txretries; tx_cnt > 0; tx_cnt--)
  {
    pkt.Buffer = (void*) tx;
    pkt.Length = length;
    if (PacketSendPacket (adapter, &pkt, TRUE))
    {
      rc = length;
      break;
    }
    STAT (macstats.num_tx_retry++);
  }

  if (rc == length)
  {
    num_tx_pkt++;
    num_tx_bytes += length;
  }
  else
  {
    num_tx_errors++;  /* local copy */
    STAT (macstats.num_tx_err++);
  }
  PROFILE_STOP();
  return (rc);
}

/*
 * Sets the receive mode of the interface.
 */
int pkt_set_rcv_mode (int mode)
{
  struct {
    PACKET_OID_DATA oidData;
    DWORD           filter;
  } oid;
  BOOL  rc;
  const ADAPTER *adapter;

  if (!_pkt_inf)
     return (0);

  adapter = (const ADAPTER*) _pkt_inf->adapter;

  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = OID_GEN_CURRENT_PACKET_FILTER;
  oid.oidData.Length = sizeof(oid.filter);
  *(DWORD*)oid.oidData.Data = mode;
  rc = PacketRequest (adapter, TRUE, &oid.oidData);
  if (rc)
       _pkt_rxmode = mode;
  else _pkt_errno = GetLastError();

  TCP_CONSOLE_MSG (2, ("pkt_set_rcv_mode(); mode 0x%02X, rc %d; %s\n",
                   mode, rc, !rc ? win_strerror(GetLastError()) : "okay"));
  return (rc);
}

/*
 * Gets the receive mode of the interface.
 *  \retval !0 Okay  - _pkt_errno = 0, _pkt_rxmode and retval is current mode.
 *  \retval 0  Error - _pkt_errno = GetLastError().
 */
int pkt_get_rcv_mode (void)
{
  struct {
    PACKET_OID_DATA oidData;
    DWORD           filter;
  } oid;
  DWORD mode;
  BOOL  rc;
  const ADAPTER *adapter;

  if (!_pkt_inf)
     return (0);

  adapter = (const ADAPTER*) _pkt_inf->adapter;

  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = OID_GEN_CURRENT_PACKET_FILTER;
  oid.oidData.Length = sizeof(oid.filter);
  rc = PacketRequest (adapter, FALSE, &oid.oidData);
  mode = *(DWORD*)oid.oidData.Data;
  if (rc)
       _pkt_rxmode = mode;
  else _pkt_errno = GetLastError();
  return (rc);
}

/*
 * Return traffic statisistics since started (stats) and
 * total since adapter was opened.
 *
 * \note 'stats' only count traffic we received and transmitted.
 */
int pkt_get_stats (struct PktStats *stats, struct PktStats *total)
{
  const ADAPTER  *adapter;
  struct bpf_stat b_stat;
  struct {
    PACKET_OID_DATA oidData;
    DWORD           value;
  } oid;
  DWORD count[4];

  if (!_pkt_inf)
     return (0);

  adapter = (const ADAPTER*) _pkt_inf->adapter;

  if (!PacketGetStatsEx(adapter,&b_stat))
     return (0);

  memset (total, 0, sizeof(*total));  /* we don't know yet */
  memset (&count, 0, sizeof(count));

  /*
   * Query: OID_GEN_RCV_OK, OID_GEN_RCV_ERROR
   *        OID_GEN_XMIT_OK, OID_GEN_XMIT_ERROR
   */
  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = OID_GEN_RCV_OK;
  oid.oidData.Length = sizeof(oid.value);
  if (!PacketRequest (adapter, FALSE, &oid.oidData))
     goto no_total;
  count[0] = *(DWORD*) &oid.oidData.Data;

  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = OID_GEN_XMIT_OK;
  oid.oidData.Length = sizeof(oid.value);
  if (!PacketRequest (adapter, FALSE, &oid.oidData))
     goto no_total;
  count[1] = *(DWORD*) &oid.oidData.Data;

  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = OID_GEN_RCV_ERROR;
  oid.oidData.Length = sizeof(oid.value);
  if (!PacketRequest (adapter, FALSE, &oid.oidData))
     goto no_total;
  count[2] = *(DWORD*) &oid.oidData.Data;

  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = OID_GEN_XMIT_ERROR;
  oid.oidData.Length = sizeof(oid.value);
  if (!PacketRequest (adapter, FALSE, &oid.oidData))
     goto no_total;
  count[3] = *(DWORD*) &oid.oidData.Data;

no_total:
  total->in_packets  = count[0];
  total->out_packets = count[1];
  total->in_errors   = count[2];
  total->out_errors  = count[3];

  stats->in_packets  = b_stat.bs_recv;
  stats->in_bytes    = num_tx_bytes;
  stats->out_packets = num_tx_pkt;
  stats->out_bytes   = num_tx_bytes;
  stats->in_errors   = 0UL;
  stats->out_errors  = num_tx_errors;
  stats->lost        = b_stat.bs_drop + b_stat.ps_ifdrop;
  return (1);
}

DWORD pkt_dropped (void)
{
  if (_pkt_inf)
     return (_pkt_inf->pkt_queue.num_drop);
  return (pkt_drop_cnt);    /* return last known value */
}

int pkt_get_mtu (void)
{
  DWORD MTU;

  if (get_interface_mtu(&MTU))
     return (MTU);
  return (0);
}

const char *pkt_strerror (int code)
{
  ARGSUSED (code);
  return (NULL);
}

#if defined(USE_MULTICAST)
/*
 * Don't think we need to support this since we can use promiscous mode
 */
int pkt_get_multicast_list (mac_address *listbuf, int *len)
{
  _pkt_errno = PDERR_NO_MULTICAST;
  *len = 0;
  ARGSUSED (listbuf);
  ARGSUSED (len);
  return (0);
}

int pkt_set_multicast_list (const void *listbuf, int len)
{
  _pkt_errno = PDERR_NO_MULTICAST;
  ARGSUSED (listbuf);
  ARGSUSED (len);
  return (0);
}
#endif


/*
 * Low-level NDIS/Mini-driver functions.
 */
static BOOL get_perm_mac_address (void *mac)
{
  struct {
    PACKET_OID_DATA oidData;
    mac_address     mac;
  } oid;
  const ADAPTER *adapter;

  if (!_pkt_inf)
     return (FALSE);

  adapter = (const ADAPTER*) _pkt_inf->adapter;
  memset (&oid, 0, sizeof(oid));

  switch (_pktdevclass)
  {
    case PDCLASS_ETHER:
         oid.oidData.Oid = OID_802_3_PERMANENT_ADDRESS;
         break;
    case PDCLASS_TOKEN:
         oid.oidData.Oid = OID_802_5_PERMANENT_ADDRESS;
         break;
    case PDCLASS_FDDI:
         oid.oidData.Oid = OID_FDDI_LONG_PERMANENT_ADDRESS;
         break;
    case PDCLASS_ARCNET:
         oid.oidData.Oid = OID_ARCNET_PERMANENT_ADDRESS;
         break;
#if 0
    case NdisMediumWan:
         oid.oidData.Oid = OID_WAN_PERMANENT_ADDRESS;
         break;
    case NdisMediumWirelessWan:
         oid.oidData.Oid = OID_WW_GEN_PERMANENT_ADDRESS;
         break;
#endif
    default:
         return (FALSE);
  }
  oid.oidData.Length = sizeof(oid.mac);

  if (!PacketRequest(adapter, FALSE, &oid.oidData))
     return (FALSE);
  memcpy (mac, &oid.oidData.Data, sizeof(mac_address));
  return (TRUE);
}

static BOOL get_interface_mtu (DWORD *mtu)
{
  struct {
    PACKET_OID_DATA oidData;
    DWORD           mtu;
  } oid;
  const ADAPTER *adapter;

  if (!_pkt_inf)
     return (FALSE);

  adapter = (const ADAPTER*) _pkt_inf->adapter;

  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = OID_GEN_MAXIMUM_TOTAL_SIZE;
  oid.oidData.Length = sizeof(oid.mtu);

  if (!PacketRequest(adapter, FALSE, &oid.oidData))
     return (FALSE);
  *mtu = *(DWORD*) &oid.oidData.Data;
  return (TRUE);
}

static BOOL get_interface_type (WORD *type)
{
  struct {
    PACKET_OID_DATA oidData;
    DWORD           link;
  } oid;
  const ADAPTER *adapter;

  if (!_pkt_inf)
     return (FALSE);

  adapter = (const ADAPTER*) _pkt_inf->adapter;

  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = OID_GEN_MEDIA_IN_USE;
  oid.oidData.Length = sizeof(oid.link);

  if (!PacketRequest(adapter, FALSE, &oid.oidData))
     return (FALSE);

  *type = *(WORD*) &oid.oidData.Data;
  return (TRUE);
}

#if defined(USE_DEBUG)
static BOOL get_interface_speed (DWORD *speed)
{
  struct {
    PACKET_OID_DATA oidData;
    DWORD           speed;
  } oid;
  const ADAPTER *adapter;

  if (!_pkt_inf)
     return (FALSE);

  adapter = (const ADAPTER*) _pkt_inf->adapter;

  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = OID_GEN_LINK_SPEED;
  oid.oidData.Length = sizeof(oid.speed);

  if (!PacketRequest(adapter, FALSE, &oid.oidData))
     return (FALSE);

  *speed = (*(DWORD*)&oid.oidData.Data) / 10000;
  return (TRUE);
}

static BOOL get_real_media (int *media)
{
  struct {
    PACKET_OID_DATA oidData;
    DWORD           media;
  } oid;
  const ADAPTER *adapter;

  if (!_pkt_inf)
     return (FALSE);

  adapter = (const ADAPTER*) _pkt_inf->adapter;
  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = OID_GEN_PHYSICAL_MEDIUM;
  oid.oidData.Length = sizeof(oid.media);

  if (!PacketRequest(adapter, FALSE, &oid.oidData))
     return (FALSE);
  *media = *(int*) &oid.oidData.Data;
  return (TRUE);
}
#endif

static BOOL get_connected_status (BOOL *is_up)
{
  struct {
    PACKET_OID_DATA oidData;
    DWORD           connected;
  } oid;
  const ADAPTER *adapter;

  if (!_pkt_inf)
     return (FALSE);

  adapter = (const ADAPTER*) _pkt_inf->adapter;

  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = OID_GEN_MEDIA_CONNECT_STATUS;
  oid.oidData.Length = sizeof(oid.connected);

  if (!PacketRequest(adapter, FALSE, &oid.oidData))
     return (FALSE);
  *is_up = (*(DWORD*) &oid.oidData.Data == NdisMediaStateConnected);
  return (TRUE);
}

#ifdef NOT_USED
/*
 * NDIS has the annoying feature (?) of looping packets we send in
 * NDIS_PACKET_TYPE_ALL_LOCAL mode (the default?). Disable it, but
 * doesn't seem to have any effect yet. Maybe need to use a BPF filter?
 *
 * No need when using RXMODE_DEFAULT (9).
 */
static BOOL ndis_set_loopback (BOOL enable)
{
  struct {
    PACKET_OID_DATA oidData;
    DWORD  options;
  } oid;
  const ADAPTER *adapter;
  BOOL  rc;
  DWORD options;
  DWORD gen_oid = OID_GEN_MAC_OPTIONS;
  DWORD opt_bit = NDIS_MAC_OPTION_NO_LOOPBACK;

  if (!_pkt_inf)
     return (FALSE);

  adapter = (const ADAPTER*) _pkt_inf->adapter;
  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = gen_oid;
  oid.oidData.Length = sizeof(oid.options);

  /* Get current MAC/protocol options
   */
  rc = PacketRequest (adapter, FALSE, &oid.oidData);
  options = *(DWORD*) &oid.oidData.Data;

 TCP_CONSOLE_MSG (2, ("ndis_set_loopback(); rc %d, options 0x%02lX; %s\n",
                  rc, options, !rc ? win_strerror(GetLastError()) : "okay"));
  if (enable)
       options &= ~opt_bit;
  else options |= opt_bit;

  /* Set it back
   */
  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = gen_oid;
  oid.oidData.Length = sizeof(oid.options);
  *(DWORD*) &oid.oidData.Data = options;
  rc = PacketRequest (adapter, TRUE, &oid.oidData);

 TCP_CONSOLE_MSG (2, ("ndis_set_loopback(); rc %d; %s\n",
                  rc, !rc ? win_strerror(GetLastError()) : "okay"));
  return (rc);
}
#endif /* NOT_USED */


/*
 * Return NDIS version as MSB.LSB.
 */
int pkt_get_api_ver (DWORD *ver)
{
  struct {
    PACKET_OID_DATA oidData;
    DWORD           version;
  } oid;
  BOOL  rc;
  const ADAPTER *adapter;

  if (!_pkt_inf)
     return (0);

  adapter = (const ADAPTER*) _pkt_inf->adapter;
  memset (&oid, 0, sizeof(oid));
  oid.oidData.Length = sizeof(oid.version);
  oid.oidData.Oid    = OID_GEN_DRIVER_VERSION;

  rc = PacketRequest (adapter, FALSE, &oid.oidData);
  if (rc)
    *ver = *(WORD*) &oid.oidData.Data;  /* only 16-bit in version */
  return (rc);
}

/*
 * Returns NPF.SYS version as major.minor.0.build (8 bits each).
 */
int pkt_get_drvr_ver (DWORD *ver)
{
  int major, minor, unused, build;

  if (sscanf(PacketGetDriverVersion(), "%d,%d,%d,%d",
             &major, &minor, &unused, &build) != 4)
     return (0);
  *ver = ((major << 24) + (minor << 16) + build);
  return (1);
}

/*
 * Show some adapter details.
 * The IP-address, netmask etc. are what Winsock uses. We ignore
 * them and use what WATTCP.CFG specifies.
 */
static void show_link_details (void)
{
#if defined(USE_DEBUG)
  const ADAPTER_INFO *ai;
  int   i, real_media;
  BOOL  is_up;
  DWORD ver, MTU, speed;
  mac_address mac;

  if (!_pkt_inf)
     return;

  ai = (const ADAPTER_INFO*) _pkt_inf->adapter_info;
  if (!ai)
  {
    (*_printf) ("  not available\n");
    return;
  }

  (*_printf) ("  %d network address%s:\n",
              ai->NNetworkAddresses, ai->NNetworkAddresses > 1 ? "es" : "");
  for (i = 0; i < ai->NNetworkAddresses; i++)
  {
    const npf_if_addr        *if_addr = ai->NetworkAddresses + i;
    const struct sockaddr_in *ip_addr = (const struct sockaddr_in*) &if_addr->IPAddress;
    const struct sockaddr_in *netmask = (const struct sockaddr_in*) &if_addr->SubnetMask;
    const struct sockaddr_in *brdcast = (const struct sockaddr_in*) &if_addr->Broadcast;

    (*_printf) ("  IP-addr %s", inet_ntoa(ip_addr->sin_addr));
    (*_printf) (", netmask %s", inet_ntoa(netmask->sin_addr));
    (*_printf) (", broadcast %s\n", inet_ntoa(brdcast->sin_addr));
  }
  if (ai->NNetworkAddresses <= 0)
     (*_printf) ("\n");

  if (get_perm_mac_address(&mac))
     (*_printf) ("  MAC-addr %02X:%02X:%02X:%02X:%02X:%02X, ",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  if (get_interface_mtu(&MTU))
     (*_printf) ("MTU %lu, ", MTU);

  (*_printf) ("link-type %s, ",
              list_lookup(_pktdevclass, logical_media, DIM(logical_media)));

  if (get_real_media(&real_media))
     (*_printf) (" over %s, ", list_lookup(real_media, phys_media, DIM(phys_media)));

  if (get_connected_status(&is_up))
     (*_printf) ("%s, ", is_up ? "UP" : "DOWN");

  if (get_interface_speed(&speed))
     (*_printf) ("%luMb/s, ", speed);

  if (pkt_get_api_ver(&ver))
     (*_printf) ("NDIS %d.%d", hiBYTE(ver), loBYTE(ver));
  (*_printf) ("\n");
#endif
}
#endif  /* WIN32 */

