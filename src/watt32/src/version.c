/*!\file version.c
 *
 * Return Watt-32 version/capabilities strings.
 */
#include <stdio.h>
#include <string.h>

#include "wattcp.h"
#include "wdpmi.h"
#include "misc.h"

#undef init_misc
#undef Random

#include <tcp.h>  /* WATTCP_MAJOR_VER, WATTCP_MINOR_VER */

#if defined(WIN32)
  #if defined(_DEBUG)
    #define DBG_RELEASE  "debug"   /* Only with MSVC */
  #else
    #define DBG_RELEASE  "release"
  #endif
  #undef  VARIANT
  #define VARIANT   " Win32"

#elif defined(_WIN64)
  #if defined(_DEBUG)
    #define DBG_RELEASE  "debug"
  #else
    #define DBG_RELEASE  "release"
  #endif
  #undef  VARIANT
  #define VARIANT   " Win64"

#elif defined(WATT32_DOS_DLL)     /* not yet */
  #ifdef __DJGPP__
    #define VARIANT  " DXE"
  #else
    #define VARIANT  " DLL"
  #endif

#else
  #define VARIANT   ""
#endif

const char *wattcpCopyright = "See COPYRIGHT.H for details";

#if defined(MAKE_TSR)
const char *wattcpVersion (void)
{
  static char buf[15];
  char  *p = buf + 8;

  strcat (buf, "Watt-32 ");
  *p++ = '0' + WATTCP_MAJOR_VER;
  *p++ = '.';
  *p++ = '0' + WATTCP_MINOR_VER;
  *p = '\0';
  return (buf);
}

#else
const char *wattcpVersion (void)
{
  static char buf[80];
  char  *p = buf;

#if defined(WATTCP_DEVEL_REL) && (WATTCP_DEVEL_REL > 0)
  p += sprintf (p, "Watt-32 (%d.%d.%d",
                WATTCP_MAJOR_VER, WATTCP_MINOR_VER, WATTCP_DEVEL_REL);
#else
  p += sprintf (p, "Watt-32 (%d.%d",
                WATTCP_MAJOR_VER, WATTCP_MINOR_VER);
#endif

  p += sprintf (p, "%s), ", VARIANT);

#if defined(__GNUC__)
  p += sprintf (p, "GNU-C %d.%d", __GNUC__, __GNUC_MINOR__);

  #if defined(__GNUC_PATCHLEVEL__) && (__GNUC_PATCHLEVEL__ > 0)
    p += sprintf (p, ".%d", __GNUC_PATCHLEVEL__);
  #endif

  /* The `__tune_xx' defines were introduced in gcc 2.95.1 (?)
   */
  #if defined (__tune_i386__)         /* -mcpu=i386 */
    strcpy (p, " (386), ");
  #elif defined (__tune_i486__)       /* -mcpu=i486 */
    strcpy (p, " (486), ");
  #elif defined (__tune_pentium__)    /* -mcpu=pentium */
    strcpy (p, " (Pent), ");
  #elif defined (__tune_pentiumpro__) /* -mcpu=pentiumpro */
    strcpy (p, " (PentPro), ");
  #else
    strcpy (p, ", ");
  #endif

#elif defined(__HIGHC__)
  strcpy (p, "Metaware High-C, ");   /* no way to get version */

#elif defined(__POCC__)
  p += sprintf (p, "PellesC %d.%d, ", __POCC__/100, __POCC__ % 100);

#elif defined(__WATCOMC__)
  #if (__WATCOMC__ >= 1200)
    p += sprintf (p, "OpenWatcom %d.%d",
                  (__WATCOMC__/100) - 11, (__WATCOMC__ % 100) / 10);
  #else
    p += sprintf (p, "Watcom C %d.%d", __WATCOMC__/100, __WATCOMC__ % 100);
  #endif

  #if defined(__SMALL__)
    strcpy (p, " (small model), ");
  #elif defined(__LARGE__)
    strcpy (p, " (large model), ");
  #else
    #if (_M_IX86 >= 600)              /* -6r/s */
      strcpy (p, " (686");
    #elif (_M_IX86 >= 500)            /* -5r/s */
      strcpy (p, " (586");
    #elif (_M_IX86 >= 400)            /* -4r/s */
      strcpy (p, " (486");
    #else                             /* -3r/s */
      strcpy (p, " (386");
    #endif

    p += 5;

    #if defined(__SW_3S)              /* -3/4/5/6s */
      strcpy (p, "S), ");
    #else
      strcpy (p, "R), ");    /* Register based calls is default */
    #endif
 #endif

#elif defined(__BORLANDC__)
  p += sprintf (p, "Borland C %X.%X", __BORLANDC__ >> 8, __BORLANDC__ & 0xFF);

  #if defined(__SMALL__)
    strcpy (p, " (small model), ");
  #elif defined(__LARGE__)
    strcpy (p, " (large model), ");
  #else
    strcpy (p, ", ");
  #endif

#elif defined(__TURBOC__)
  p += sprintf (p, "Turbo C %X.%X", (__TURBOC__ >> 8) - 1, __TURBOC__ & 0xFF);

  #if defined(__SMALL__)
    strcpy (p, " (small model), ");
  #elif defined(__LARGE__)
    strcpy (p, " (large model), ");
  #else
    strcpy (p, ", ");
  #endif

#elif defined(_MSC_VER)
  #if (_MSC_VER == 1300)
    p += sprintf (p, "Microsoft Visual Studio .NET (%s), ", DBG_RELEASE);
  #elif (_MSC_VER == 1310)
    p += sprintf (p, "Microsoft Visual Studio .NET 2003 (%s), ", DBG_RELEASE);
  #elif (_MSC_VER == 1400)
    p += sprintf (p, "Microsoft Visual Studio .NET 2005 (%s), ", DBG_RELEASE);
  #elif (_MSC_VER >= 1000)     /* Visual-C 4+ */
    p += sprintf (p, "Microsoft Visual-C %d.%d (%s), ",
                  (_MSC_VER / 100) - 6, _MSC_VER % 100, DBG_RELEASE);
  #else
    p += sprintf (p, "Microsoft Quick-C %d.%d",
                  _MSC_VER / 100, _MSC_VER % 100);

    #if defined(__SMALL__)
      strcpy (p, " (small model), ");
    #elif defined(__LARGE__)
      strcpy (p, " (large model), ");
    #else
      strcpy (p, ", ");
    #endif
  #endif

#elif defined(__DMC__)
  p += sprintf (p, "Digital Mars C %X.%X", __DMC__ >> 8, __DMC__ & 0xFF);

  #if defined(DOS386)
    strcpy (p, " (32-bit), ");
  #elif defined(__SMALL__)
    strcpy (p, " (small model), ");
  #elif defined(__LARGE__)
    strcpy (p, " (large model), ");
  #else
    strcpy (p, ", ");
  #endif

#elif defined(__CCDL__)
  p += sprintf (p, "LadSoft C %d.%d, ", __CCDL__/100, __CCDL__ % 100);

#elif defined(__LCC__) && defined(WIN32)
  p += sprintf (p, "lcc-win32, ");

#elif defined(__ICC__)
  p += sprintf (p, "Intel C %d.%d, ", __ICC__/100, __ICC__ % 100);
#endif

  p = strchr (buf, '\0');

#if defined(__MINGW32__)
  sprintf (p, "MingW %d.%d, ", __MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION);

#elif defined(__CYGWIN__)   /* !! not tested */
  sprintf (p, "CygWin %d.%d, ", __CYGWIN_MAJOR_VERSION, __CYGWIN_MINOR_VERSION);

#elif (DOSX == DJGPP)
  sprintf (p, "djgpp %d.%02d, ", __DJGPP__, __DJGPP_MINOR__);

#elif (DOSX == DOS4GW)
  {
    const char *extender = dos_extender_name();
    sprintf (p, "%s, ", extender ? extender : "unknown");
  }

#elif (DOSX == PHARLAP)
  sprintf (p, "Pharlap, ");

#elif (DOSX == X32VM)
  strcpy (p, "X32VM, ");

#elif (DOSX == POWERPAK)
  strcpy (p, "PowerPak, ");
#endif

  strcat (p, __DATE__);
  return (buf);
}
#endif   /* MAKE_TSR */


/**
 * Return compiled-in capabilities.
 */
static const char *capa[] = {
#if defined(MAKE_TSR)
           "TSR",
#endif
#if defined(USE_DEBUG)
           "debug",
#endif
#if defined(USE_MULTICAST)
           "mcast",
#endif
#if defined(USE_BIND)
           "bind",
#endif
#if defined(USE_BSD_API)
           "BSDsock",
#endif
#if defined(USE_BSD_FATAL)
           "BSDfatal",
#endif
#if defined(USE_BOOTP)
           "bootp",
#endif
#if defined(USE_DHCP)
           "dhcp",
#endif
#if defined(USE_RARP)
           "rarp",
#endif
#if defined(USE_LANGUAGE)
           "lang",
#endif
#if defined(USE_FRAGMENTS)
           "frag",
#endif
#if defined(USE_STATISTICS)
           "stat",
#endif
#if defined(USE_FORTIFY)
           "fortify",
#endif
#if defined(USE_CRTDBG)
           "crt-dbg",
#endif
#if defined(USE_FSEXT)
           "fsext",
#endif
#if defined(USE_LOOPBACK)
           "loopback",
#endif
#if defined(USE_EMBEDDED)
           "embedded",
#endif
#if defined(USE_TFTP)
           "tftp",
#endif
#if defined(USE_TCP_SACK)
           "sack",
#endif
#if defined(USE_MTU_DISC)
           "MTU-disc",
#endif
#if defined(USE_UDP_ONLY)
           "udp-only",
#endif
#if defined(USE_ECHO_DISC)
           "echo",
#endif
#if defined(USE_PPPOE)
           "PPPoE",
#endif
#if defined(USE_IPV6)
           "IPv6",
#endif
#if defined(USE_RS232_DBG)
           "RS232",
#endif
#if defined(USE_DEAD_GWD)
           "dead-gw",
#endif
#if defined(USE_GZIP_COMPR)
           "gzip-compr",
#endif
#if defined(USE_SECURE_ARP)
           "sec-ARP",
#endif
#if defined(USE_TCP_MD5)
           "TCP-MD5",
#endif
#if defined(USE_DYNIP_CLI)
           "DynIP",
#endif
#if defined(USE_PROFILER)
           "profiler",
#endif
#if defined(USE_FAST_PKT)
           "fast-pkt",
#endif
           NULL
         };

const char *wattcpCapabilities (void)
{
  static char buf[240];
  char  *p = buf;
  int    i;

  for (i = 0; capa[i]; i++)
  {
    *p++ = '/';
    if (p + strlen(capa[i]) - 2 >= buf + sizeof(buf))
    {
      *p++ = '.';
      *p++ = '.';
      break;
    }
    strcpy (p, capa[i]);
    p += strlen (capa[i]);
  }
  *p = '\0';
  return (buf);
}

/*
 * Check some prototypes and decorations in <tcp.h> against the
 * local prototypes.
 */
#undef init_misc
#undef set_timeout
#undef chk_timeout
#undef hires_timer
#undef set_timediff
#undef get_timediff
#undef timeval_diff
#undef init_timer_isr
#undef exit_timer_isr
#undef has_rdtsc
#undef clocks_per_usec
#undef Random

#undef my_ip_addr
#undef sin_mask
#undef block_ip
#undef block_tcp
#undef block_udp
#undef block_icmp
#undef aton
#undef isaddr
#undef ctrace_on
#undef _mss
#undef _mtu
#undef _inet_ntoa
#undef _inet_addr
#undef _printf
#undef _outch
#undef outs
#undef outsnl
#undef outsn
#undef outhexes
#undef outhex
#undef rip
#undef survive_eth
#undef survive_bootp
#undef survive_dhcp
#undef survive_rarp
#undef sock_inactive
#undef sock_data_timeout
#undef sock_delay
#undef multihomes
#undef usr_init
#undef usr_post_init
#undef cookies
#undef last_cookie
#undef loopback_handler
#undef in_checksum
#undef parse_config_table

/* hide some prototypes from <tcp.h>
 */
#define udp_SetTTL  foo_udp_SetTTL

#include "socket.h"
