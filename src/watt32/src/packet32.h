/*
 * Copyright (c) 1999 - 2003
 * NetGroup, Politecnico di Torino (Italy)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Politecnico di Torino nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _w32_PACKET32_H
#define _w32_PACKET32_H

/* Never include old-style BPF stuff here; use the WinPcap
 * definition copied in below
 */
#define __NET_BPF_H

#ifdef __DMC__
#include <win32/wtypes.h>  /* Has <wtypes.h> in include path */
#endif

#include <netinet/in.h>    /* struct sockaddr_storage */

/* Working modes
 */
#define PACKET_MODE_CAPT      0x0                    /* Capture mode */
#define PACKET_MODE_STAT      0x1                    /* Statistical mode */
#define PACKET_MODE_MON       0x2                    /* Monitoring mode */
#define PACKET_MODE_DUMP      0x10                   /* Dump mode */
#define PACKET_MODE_STAT_DUMP (MODE_STAT|MODE_DUMP)  /* Statistical dump Mode */

/*
 * Macro definition for defining IOCTL and FSCTL function control codes.  Note
 * that function codes 0-2047 are reserved for Microsoft Corporation, and
 * 2048-4095 are reserved for customers.
 */
#define CTL_CODE(DeviceType, Function, Method, Access) ( \
        ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) )

/* Define the method codes for how buffers are passed for I/O and FS controls
 */
#define METHOD_BUFFERED    0
#define METHOD_IN_DIRECT   1
#define METHOD_OUT_DIRECT  2
#define METHOD_NEITHER     3

/*
 * Duplicates from <WinIoCtl.h>:
 * The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
 * ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
 * constants *MUST* always be in sync.
 */
#ifndef FILE_ANY_ACCESS
#define FILE_ANY_ACCESS    0
#endif

#ifndef FILE_READ_ACCESS
#define FILE_READ_ACCESS   0x0001    /* file & pipe */
#endif

#ifndef FILE_READ_ACCESS
#define FILE_WRITE_ACCESS  0x0002    /* file & pipe */
#endif

/* ioctls
 */
#ifndef FILE_DEVICE_PROTOCOL
#define FILE_DEVICE_PROTOCOL          0x8000
#endif

#ifndef FILE_DEVICE_PHYSICAL_NETCARD
#define FILE_DEVICE_PHYSICAL_NETCARD  0x0017
#endif

#define IOCTL_PROTOCOL_STATISTICS CTL_CODE (FILE_DEVICE_PROTOCOL, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROTOCOL_RESET      CTL_CODE (FILE_DEVICE_PROTOCOL, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROTOCOL_READ       CTL_CODE (FILE_DEVICE_PROTOCOL, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROTOCOL_WRITE      CTL_CODE (FILE_DEVICE_PROTOCOL, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROTOCOL_MACNAME    CTL_CODE (FILE_DEVICE_PROTOCOL, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_OPEN                CTL_CODE (FILE_DEVICE_PROTOCOL, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CLOSE               CTL_CODE (FILE_DEVICE_PROTOCOL, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define	pBIOCSETBUFFERSIZE        9592   /* set kernel buffer size */
#define	pBIOCSETF                 9030   /* set packet filtering program */
#define pBIOCGSTATS               9031   /* get the capture stats */
#define	pBIOCSRTIMEOUT            7416   /* set the read timeout */
#define	pBIOCSMODE                7412   /* set working mode */
#define	pBIOCSWRITEREP            7413   /* set number of physical repetions of every packet written by the app. */
#define	pBIOCSMINTOCOPY           7414   /* set minimum amount of data in the kernel buffer that unlocks a read call */
#define	pBIOCSETOID        2147483648U   /* set an OID value */
#define	pBIOCQUERYOID      2147483652U   /* get an OID value */
#define	pATTACHPROCESS            7117   /* attach a process to the driver. Used in Win9x only */
#define	pDETACHPROCESS            7118   /* detach a process from the driver. Used in Win9x only */
#define pBIOCEVNAME               7415   /* get the name of the event that the driver signals when some data is present in the buffer */
#define pBIOCSETDUMPFILENAME      9029   /* set the name of a the file used by kernel dump mode */
#define pBIOCSENDPACKETSNOSYNC    9032   /* send a buffer containing multiple packets to the network, ignoring the timestamps associated with the packets */
#define pBIOCSENDPACKETSSYNC      9033   /* send a buffer containing multiple packets to the network, respecting the timestamps associated with the packets */
#define pBIOCSETDUMPLIMITS        9034   /* set the dump file limits. See the PacketSetDumpLimits() function */

/* Alignment macro. Defines the alignment size.
 */
#define Packet_ALIGNMENT sizeof(int)

/* Alignment macro. Rounds up to the next even multiple of Packet_ALIGNMENT.
 */
#define Packet_WORDALIGN(x) (((x) + (Packet_ALIGNMENT-1)) & ~(Packet_ALIGNMENT-1))

#define NdisMediumNull	    -1   /* Custom linktype: NDIS doesn't provide an equivalent */
#define NdisMediumCHDLC	    -2   /* Custom linktype: NDIS doesn't provide an equivalent */
#define NdisMediumPPPSerial -3   /* Custom linktype: NDIS doesn't provide an equivalent */

/**
 * Addresses of a network adapter.
 *
 * This structure is used by the PacketGetNetInfoEx() function to return
 * the IP addresses associated with an adapter.
 */
typedef struct npf_if_addr {
        struct sockaddr_storage IPAddress;
        struct sockaddr_storage SubnetMask;
        struct sockaddr_storage Broadcast;
      } npf_if_addr;


#define MAX_LINK_NAME_LENGTH   64       /* Maximum length of the devices symbolic links */
#define ADAPTER_NAME_LENGTH   (256+12)  /* Max length for the name of an adapter */
#define ADAPTER_DESC_LENGTH    128      /* Max length for the description of an adapter */
#define MAX_MAC_ADDR_LENGTH    8        /* Max length for the link layer address */
#define MAX_NETWORK_ADDRESSES  16       /* Max # of network layer addresses */

/**
 * Contains comprehensive information about a network adapter.
 *
 * This structure is filled with all the accessory information that the
 * user can need about an adapter installed on his system.
 */
typedef struct ADAPTER_INFO {
        struct ADAPTER_INFO *Next;
        char                 Name [ADAPTER_NAME_LENGTH+1];
        char                 Description [ADAPTER_DESC_LENGTH+1];
        int                  NNetworkAddresses;
        struct npf_if_addr   NetworkAddresses [MAX_NETWORK_ADDRESSES];
      } ADAPTER_INFO;

/**
 * Describes an opened network adapter.
 */
typedef struct ADAPTER {
        HANDLE hFile;
        HANDLE ReadEvent;
        UINT   ReadTimeOut;
        char   Name [ADAPTER_NAME_LENGTH];          /**< \todo: remove this */
     /* struct ADAPTER_INFO *info;  \todo */
      } ADAPTER;


/**
 * Structure for PacketReceivePacket + PacketSendPacket.
 */
typedef struct PACKET {
        HANDLE      hEvent;
        OVERLAPPED  OverLapped;
        void       *Buffer;
        UINT        Length;
        DWORD       ulBytesReceived;
        BOOLEAN     bIoComplete;
      } PACKET;

/**
 * Structure containing an OID request.
 *
 * It is used by the PacketRequest() function to send an OID to the interface
 * card driver. It can be used, for example, to retrieve the status of the error
 * counters on the adapter, its MAC address, the list of the multicast groups
 * defined on it, and so on.
 */
typedef struct PACKET_OID_DATA {
        DWORD  Oid;
        DWORD  Length;
        BYTE   Data[1];
      } PACKET_OID_DATA;

/**
 * Berkeley Packet Filter header.
 *
 * This structure defines the header associated with every packet delivered to the application.
 */
struct bpf_hdr {
       struct timeval bh_tstamp;  /* The timestamp associated with the captured packet */
       DWORD          bh_caplen;  /* Length of captured portion */
       DWORD          bh_datalen; /* Original length of packet */
       WORD           bh_hdrlen;  /* Length of bpf header (this struct plus alignment padding) */
     };

/* Include the BPF structures here since the WinPcap headers messes
 * this up big time.
 */
struct bpf_stat {
       DWORD bs_recv;   /* # of packets that the driver received from the network adapter */
       DWORD bs_drop;   /* # of packets that the driver lost from the beginning of a capture */
       DWORD ps_ifdrop;	/* drops by interface. XXX not yet supported */
       DWORD bs_capt;	/* # of packets that pass the filter */
     };

/* The instruction data structure.
 */
struct bpf_insn {
       WORD  code;
       BYTE  jt;
       BYTE  jf;
       DWORD k;
    };

/* Structure for BIOCSETF.
 */
struct bpf_program {
       DWORD            bf_len;
       struct bpf_insn *bf_insns;
     };

BOOL PacketInitModule (BOOL init, FILE *dump_file);

#if defined(USE_DYN_PACKET)
  struct dyn_table {
         const ADAPTER *(__cdecl *PacketOpenAdapter) (const char *);
         void           (__cdecl *PacketCloseAdapter) (ADAPTER *);
         BOOL           (__cdecl *PacketRequest) (const ADAPTER *, BOOL, PACKET_OID_DATA *);
         const char    *(__cdecl *PacketGetDriverVersion) (void);
         BOOL           (__cdecl *PacketSetMode) (const ADAPTER *, DWORD);
         BOOL           (__cdecl *PacketSetBuff) (const ADAPTER *, DWORD);
         BOOL           (__cdecl *PacketSetMinToCopy) (const ADAPTER *, int);
         BOOL           (__cdecl *PacketSetReadTimeout) (ADAPTER *, int);
         BOOL           (__cdecl *PacketGetStatsEx) (const ADAPTER *, struct bpf_stat *);
         BOOL           (__cdecl *PacketReceivePacket) (const ADAPTER *, PACKET *, BOOL);
         int            (__cdecl *PacketSendPacket) (const ADAPTER *, PACKET *, BOOL);
       };

  extern struct dyn_table dyn_funcs;

  #define PacketOpenAdapter(name)    (*dyn_funcs.PacketOpenAdapter) (name)
  #define PacketCloseAdapter(a)      (*dyn_funcs.PacketCloseAdapter) (a)
  #define PacketRequest(a,s,p)       (*dyn_funcs.PacketRequest) (a,s,p)
  #define PacketGetDriverVersion()   (*dyn_funcs.PacketGetDriverVersion)()
  #define PacketSetMode(a,m)         (*dyn_funcs.PacketSetMode) (a,m)
  #define PacketSetBuff(a,s)         (*dyn_funcs.PacketSetBuff) (a,s)
  #define PacketSetMinToCopy(a,n)    (*dyn_funcs.PacketSetMinToCopy) (a,n)
  #define PacketSetReadTimeout(a,t)  (*dyn_funcs.PacketSetReadTimeout) (a,t)
  #define PacketGetStatsEx(a,s)      (*dyn_funcs.PacketGetStatsEx) (a,s)
  #define PacketReceivePacket(a,p,s) (*dyn_funcs.PacketReceivePacket) (a,p,s)
  #define PacketSendPacket(a,p,s)    (*dyn_funcs.PacketSendPacket) (a,p,s)

#else

  const ADAPTER      *PacketOpenAdapter (const char *AdapterName);
  const ADAPTER_INFO *PacketFindAdInfo (const char *AdapterName);
  const ADAPTER_INFO *PacketGetAdInfo (void);
  void                PacketCloseAdapter (ADAPTER *AdapterObject);
  BOOL                PacketRequest (const ADAPTER *AdapterObject, BOOL Set, PACKET_OID_DATA *OidData);
  const char         *PacketGetDriverVersion (void);

  BOOL PacketSetMode (const ADAPTER *AdapterObject, DWORD mode);
  BOOL PacketSetBuff (const ADAPTER *AdapterObject, DWORD dim);
  BOOL PacketSetMinToCopy (const ADAPTER *AdapterObject, int nbytes);
  BOOL PacketSetReadTimeout (ADAPTER *AdapterObject, int timeout);
  BOOL PacketGetStatsEx (const ADAPTER *AdapterObject, struct bpf_stat *st);
  BOOL PacketReceivePacket (const ADAPTER *AdapterObject, PACKET *pkt, BOOL sync);
  int  PacketSendPacket (const ADAPTER *AdapterObject, PACKET *pkt, BOOL sync);

#endif  /* USE_DYN_PACKET */
#endif  /* _w32_PACKET32_H */
