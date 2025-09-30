/*!\file sock_ini.c
 *
 * Watt-32 initialisation.
 *
 * watt_sock_init() - easy way to guarentee:
 *  - card is ready
 *  - shutdown is handled
 *  - ^C/^BREAK are handled
 *  - WATTCP.CFG config file is read
 *  - BOOTP/PCDHCP/RARP is run
 *
 * \version 0.1: May 2, 1991  Erick - reorganized operations.
 *
 * \version 0.2: 1998 Gisle V. - Major rewrite; added DHCP, additional
 *               startup checks for various targets. Exception handler
 *               releases PKTDRVR.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <io.h>

#ifdef __HIGHC__
#include <init.h>  /* _mwenv(), PL_ENV */
#endif

#if defined (__DJGPP__)
#include <sys/exceptn.h>
#endif

#include "copyrigh.h"
#include "wattcp.h"
#include "strings.h"
#include "language.h"
#include "misc.h"
#include "timer.h"
#include "wdpmi.h"
#include "x32vm.h"
#include "powerpak.h"
#include "pcbootp.h"
#include "pcrarp.h"
#include "pcdhcp.h"
#include "pcconfig.h"
#include "pcmulti.h"
#include "pcicmp.h"
#include "pctcp.h"
#include "pcarp.h"
#include "pcsed.h"
#include "pcqueue.h"
#include "pcpkt.h"
#include "pcdbug.h"
#include "netaddr.h"
#include "syslog2.h"
#include "udp_dom.h"
#include "bsdname.h"
#include "bsddbug.h"
#include "ip4_frag.h"
#include "ip6_in.h"
#include "printk.h"
#include "sock_ini.h"

#if (DOSX & PHARLAP)
#include <mw/exc.h>
#endif

#if defined(WATT32_DOS_DLL)
/* #include <exc.h> */
#endif

#if defined(USE_TFTP)
#include "tftp.h"
#endif

#if defined(USE_BSD_API)
#include "socket.h"
#include "get_xby.h"
#endif

#if defined(USE_BIND)
#include "resolver.h"
#endif

#if defined(USE_ECHO_DISC)
#include "echo.h"
#endif

#if defined(USE_PPPOE)
#include "pppoe.h"
#endif

#if defined(USE_IDNA)
#include "idna.h"
#endif

#if defined(USE_DYNIP_CLI)
#include "dynip.h"
#endif

#if defined(WIN32)
#include "winpcap.h"
#endif

#if defined(__HIGHC__)  /* Metaware's HighC have SIGBREAK == SIGQUIT */
#undef SIGBREAK
#endif

#include "nochkstk.h"

BOOL _bootp_on       = FALSE; /**< Try booting using BOOTP ? */
BOOL _dhcp_on        = FALSE; /**< Try booting using DHCP ? */
BOOL _dhcp6_on       = FALSE; /**< Try booting using DHCP6 ? */
BOOL _rarp_on        = FALSE; /**< Try booting using RARP ? */
BOOL _do_mask_req    = FALSE; /**< do an "ICMP Mask Request" when configured */
BOOL _watt_do_exit   = TRUE;  /**< exit program when all boot attempts failed */
BOOL _watt_no_config = FALSE; /**< run with no config file (embedded/diskless) */

WattUserConfigFunc _watt_user_config_fn = NULL;

BOOL survive_eth = 0; /**< GvB 2002-09, allows us to survive without a
                       *   (working) packet driver at all - in cases where life
                       *   still has a meaning without TCP/IP.
                       */

#if defined(USE_DHCP)
  BOOL survive_bootp = TRUE;     /**< Survive a failed BOOTP attempt */
#else
  BOOL survive_bootp = FALSE;    /**< Don't survive a failed BOOTP attempt */
#endif

#if defined(USE_RARP)
  BOOL survive_dhcp = TRUE;      /**< Survive a failed DHCP attempt */
#else
  BOOL survive_dhcp = FALSE;     /**< Don't survive a failed DHCP attempt */
#endif

BOOL survive_rarp = FALSE;       /**< Don't survive a failed RARP attempt */
BOOL _watt_is_init     = FALSE;  /**< watt_sock_init() done (but with possible failed boot) */

static BOOL tcp_is_init = FALSE; /**< tcp_init() called okay. */
static int  old_break   = -1;    /**< Original state of DOS's BREAK handler. */

/**
 * A user application may call the `_watt_user_config()' function prior to
 * calling sock_init() to inject its own configuration values into the
 * tcp_config() parser.
 * Useful when running diskless and setting `_watt_no_config = 0'.
 *
 * Example:
 *  - 'pass' is 1 for first pass (PKTDRVR/WinPcap configuration), 2 for second pass.
 *  - 'cfg' is a pointer to the configuration table and must be passed to
 *    tcp_inject_config() as-is.
 *
 * In most cases 'pass' can probably be ignored.
 *
 * static long my_config_fn (int pass, const struct config_table *cfg)
 * {
 *   // Keyword case is not significant (tcp_inject_config will convert
 *   // all keys to uppercase)
 *
 *   tcp_inject_config (cfg, "My_IP", "192.168.0.98");
 *   tcp_inject_config (cfg, "NetMask", "255.255.255.0");
 *   tcp_inject_config (cfg, "Gateway", "192.168.0.1");
 *   (void)pass;
 *   return (1); // Returning 0 would make tcp_config() return 0 too,
 *               // as if the config file was not found
 * }
 *
 * int main (void)
 * {
 *   _watt_no_config = 1;
 *   _watt_user_config (my_config_fn);
 *   if (sock_init())
 *      return (-1);
 *   ...
 * }
 *
 * Never called inside Watt-32.
 */
WattUserConfigFunc _watt_user_config (WattUserConfigFunc fn)
{
  WattUserConfigFunc old_fn = _watt_user_config_fn;

  _watt_user_config_fn = fn;
  return (old_fn);
}

/**
 * Exit application if _watt_do_exit is TRUE (the default).
 * Otherwise return 'code'. Only called during watt_sock_init().
 */
static int do_exit (int code)
{
  if (_watt_do_exit)
     exit (code);
  return (code);
}

/**
 * Some target dependent functions. Install signal handlers.
 */
#if !defined(MAKE_TSR)

static BOOL use_except = TRUE;

#if (DOSX & (DOS4GW|X32VM))

static jmp_buf            exc_jmp;
static struct FAULT_STRUC exc_buf;

static void pre_except_handler (const struct FAULT_STRUC *exc)
{
  exc_buf = *exc;
  longjmp (exc_jmp, SIGSEGV);
}

#if !defined(__CCDL__) && !defined(__BORLANDC__)
static void do_traceback (void)
{
  _printk_safe = TRUE;
  _printk ("\r\nException %d at %04X:%08X\r\n",
           exc_buf.fault_num, exc_buf.cs, exc_buf.eip);

  _printk ("  EAX %08X  EBX %08X  ECX %08X  EDX %08X\r\n",
           exc_buf.eax, exc_buf.ebx, exc_buf.ecx, exc_buf.edx);

  _printk ("  ESI %08X  EDI %08X  EBP %08X  ESP %08X\r\n",
           exc_buf.esi, exc_buf.edi, exc_buf.ebp, exc_buf.esp);

  _printk ("  DS %04X  ES %04X  FS %04X  GS %04X  SS %04X  EFLAGS %08X\r\n",
           exc_buf.ds, exc_buf.es, exc_buf.fs, exc_buf.gs, exc_buf.ss,
           exc_buf.eflags);

  _printk_flush();

  if (!exc_buf.mode)  /* exception in protected mode */
  {
    stack_rewind (exc_buf.eip, exc_buf.ebp);
    _printk_flush();
  }
}
#endif   /* __CCDL__ */
#endif   /* DOS4GW|X32VM */


#if defined (__DJGPP__)
static void except_handler (int sig)
{
  static BOOL been_here = FALSE;
  static jmp_buf exc_buf;

  _watt_fatal_error = TRUE;

  if (been_here)
  {
    been_here = FALSE;
    signal (sig, SIG_DFL);
    __djgpp_exception_state_ptr = &exc_buf;
  }
  else  /* save exception context in case of reentry */
  {
    memcpy (&exc_buf, __djgpp_exception_state_ptr, sizeof(exc_buf));
    been_here = TRUE;
    psignal (sig, "TCP/IP shutdown");
    rundown_run();

#if 0
    /** \todo Disassemble crash address */
    StackWalk (exc_buf[0].__eip, exc_buf[0].__ebp);
#endif
  }
  raise (SIGABRT);
}

#elif (DOSX & PHARLAP)
static void except_handler (excReg *regs)
{
  _watt_fatal_error = TRUE;
  rundown_run();
  _exit (-1);
  ARGSUSED (regs);
}

#else
/*
 * Note: Watcom's extension to SIGFPE is undocumented.
 * This is dead code on Windows since except_handler()
 * isn't installed yet.
 */
static void except_handler (int sig, int code)
{
#if defined(__WATCOMC__)
  if (sig == SIGFPE && code == FPE_IOVERFLOW)
  {
    _fpreset();
    outsnl (_LANG("Ignoring SIGFPE (FPE_IOVERFLOW)"));
    return;
  }
#endif

#if !defined(WIN32)
  /* Nothing is really fatal under Watt-32/Win32 */
  _watt_fatal_error = TRUE;
#endif

#if defined(SIGFPE)
  if (sig == SIGFPE)
  {
#if defined(__WATCOMC__)
    outs (_LANG("Trapping SIGFPE code 0x"));
    outhex (code);
#else
    outsnl (_LANG("Trapping SIGFPE."));
#endif
  }
  else
#endif

#if defined(SIGSEGV)
  if (sig == SIGSEGV)
  {
    outsnl (_LANG("Trapping SIGSEGV"));
#if (DOSX & (DOS4GW|X32VM)) && !defined(__CCDL__) && !defined(__BORLANDC__)
    do_traceback();
#endif
  }
#endif

  rundown_run();

#if (DOSX) /* don't pull in iob for small/large */
  fflush (stdout);
  fflush (stderr);
#endif

  ARGSUSED (code);
#if defined(__POCC__)
  exit (-1);
#else
  _exit (-1);
#endif
}
#endif  /* __DJGPP__ */


/**
 * Install signal-handlers for fatal errors;
 * SIGSEGV, SIGILL, SIGTRAP, SIGFPE etc.
 */
static void setup_sig_handlers (void)
{
#if (DOSX & (DOS4GW|X32VM))
  volatile int sig;
#endif

#if defined(WIN32)     /* no need for a special exc-handler (yet?) */
  use_except = FALSE;
#else
  if (getenv("WATT32-NOEXC") ||    /* don't trap exceptions */
      getenv("WATT32-NOEXCEPT"))
     use_except = FALSE;
#endif

  if (!use_except)
     return;

#if (DOSX & PHARLAP)
  InstallExcHandler (except_handler);

#elif (DOSX & X32VM)
  _printk_init (2000, NULL);
  _x32_fault_intercept (pre_except_handler);
  sig = setjmp (exc_jmp);
  if (sig)
     except_handler (sig, 0);

#elif (DOSX & DOS4GW)
  signal (SIGSEGV, (void(*)(int))except_handler);
  signal (SIGFPE, (void(*)(int))except_handler);

  _printk_init (2000, NULL);
  dpmi_except_handler (pre_except_handler); /* Only effective for Causeway */
  sig = setjmp (exc_jmp);
  if (sig)
     except_handler (sig, 0);

#elif defined(__DJGPP__)
  signal (SIGSEGV, except_handler);
  signal (SIGTRAP, except_handler);
  signal (SIGFPE, except_handler);
  signal (SIGILL, except_handler);

#else
  /*
   * SIGSEGV may not be effective under all environments.
   * PowerPak definetely supports SIGSEGV.
   */
  #ifdef SIGSEGV
    signal (SIGSEGV, (void(*)(int))except_handler);
  #endif
  #ifdef SIGFPE
    signal (SIGFPE, (void(*)(int))except_handler);
  #endif
#endif
}

static void restore_sig_handlers (void)
{
  if (!_watt_cbroke)
     signal (SIGINT, SIG_DFL);

  if (old_break >= 0)
     set_cbreak (old_break);

  if (!use_except)
     return;

#if (DOSX & DOS4GW)
  signal (SIGSEGV, SIG_DFL);
  signal (SIGFPE, SIG_DFL);

#elif defined(__DJGPP__)
  signal (SIGSEGV, SIG_DFL);
  signal (SIGTRAP, SIG_DFL);
  signal (SIGFPE, SIG_DFL);
  signal (SIGILL, SIG_DFL);

#elif defined(__BORLANDC__) && defined(SIGSEGV) && defined(SIGFPE)
  signal (SIGSEGV, SIG_DFL);
  signal (SIGFPE, SIG_DFL);
#endif
}
#endif  /* MAKE_TSR */


/**
 * Abort all TCP sockets, release DHCP lease and restore signal
 * handlers.
 */
static void tcp_shutdown (void)
{
  if (!tcp_is_init)
     return;

#if !defined(USE_UDP_ONLY)
  while (_tcp_allsocs)
        TCP_ABORT (_tcp_allsocs);   /* Sends RST if needed */
  _tcp_allsocs = NULL;
#endif
  _udp_allsocs = NULL;

#if defined(USE_DHCP)
  /*
   * If DHCP-config saved to disk and remaining lease is above min.,
   * do nothing. Otherwise be nice and release our address
   */
  DHCP_release (FALSE);
#endif

#if defined(USE_PPPOE)
  pppoe_exit();
#endif

  tcp_is_init = FALSE;
}


/**
 * Initialise the PKTDRVR (calls _eth_init()).
 *  - Reset nameserver table.
 *  - Initialise local ports.
 *  - Get machine name (w/o domain) from LAN extension (if any).
 *  - Prepare parsing TCP configuration.
 *
 * \note May be called more than once without hurting.
 */
static int tcp_init (void)
{
  int rc = WERR_NO_DRIVER;

  if (!tcp_is_init)
  {
    tcp_is_init = TRUE;
    rc = _eth_init();          /* initialize LAN-card */
    if (rc == 0)
    {
      last_nameserver  = 0;    /* reset the nameserver table */
      last_cookie = 0;         /* eat all remaining crumbs */
      if (!init_localport())   /* clear local ports in-use */
         return (WERR_NO_MEM);

      /* I not already set, try asking a DOS network extension
       * for a host-name.
       */
      if (!strcmp(hostname,"random-pc"))
         _get_machine_name (hostname, sizeof(hostname));
    }
  }
  return (rc);
}

/**
 * Initialise stuff based on configured values.
 */
static void tcp_post_init (void)
{
  DWORD max_rwin;
  int   MTU;

  if (!tcp_is_init)       /* tcp_init() not called */
     return;

#if defined(USE_BSD_API)
  ReadEthersFile();       /* requires gethostbyname() */
#endif

#if defined(USE_PROFILER)
  if (profile_enable)
     profile_init();
#endif

#if defined(USE_IPV6)
  _ip6_post_init();
#endif

#if defined(USE_DEBUG)
  if (debug_on >= 3)
     rundown_dump();
#endif

  MTU = pkt_get_mtu();    /* driver knows correct MTU (includes MAC-header) */
  if (MTU > 0)
     _mtu = MTU;

  if (_mtu > ETH_MAX_DATA)
      _mtu = ETH_MAX_DATA;

  if (_mss > MSS_MAX)
      _mss = MSS_MAX;

  /* Work around a limitation of NDIS3PKT. It only allocates 6 buffers
   * per VDD (DOS-box). Thus receiving a burts of RWIN/MSS packets, they
   * could all get lost. Clamp our advertised RWIN.
   *
   * Note: bandwidth <= 1.3 * MTU / (RTT * sqrt(Loss))
   */
  max_rwin = 6 * _mss;
  if (_eth_ndis3pkt && tcp_recv_win > max_rwin)
     tcp_recv_win = max_rwin;

  if (usr_post_init)      /* tell hook(s) we're done */
    (*usr_post_init)();
}

/**
 * Try to boot using BOOTP, DHCP or RARP.
 * Only called if at least one '_*on' flag is set.
 *
 * \retval 0 on success.
 */
static int tcp_do_boot (BOOL try_bootp, BOOL try_dhcp, BOOL try_rarp)
{
  if (try_bootp)
  {
#if defined(USE_BOOTP)
    if (BOOTP_do_boot())
       return (0);

    outsnl (_LANG("failed"));
    if (!survive_bootp)
       return (WERR_BOOTP_FAIL);
#else
    outsnl ("BOOTP needed, but library wasn't built with \"USE_BOOTP\"");
#endif

#if defined(USE_DHCP)
    try_dhcp = TRUE;
#endif

#if defined(USE_RARP)
    try_rarp = TRUE;
#endif
  }

  if (try_dhcp)
  {
#if defined(USE_DHCP)
    if (DHCP_do_boot())
       return (0);

    outsnl (_LANG("failed"));
    if (!survive_dhcp)
       return (WERR_DHCP_FAIL);
#else
    outsnl ("DHCP needed, but library wasn't built with \"USE_DHCP\"");
#endif

#if defined(USE_RARP)
    try_rarp = TRUE;
#endif
  }

  if (try_rarp)
  {
#if defined(USE_RARP)
    if (_dorarp())
       return (0);

    outsnl (_LANG("failed"));
    if (!survive_rarp)
       return (WERR_RARP_FAIL);
#else
    outsnl ("RARP needed, but library wasn't built with \"USE_RARP\"");
#endif
  }
  return (WERR_NO_IPADDR);  /* all attempts failed */
}


#if defined(USE_DEBUG)
/*
 * A guard against this common error; The wattlib user forgot to rebuild
 * using the latest <tcp.h>.
 */
static void check_sock_sizes (size_t tcp_Sock_size, size_t udp_Sock_size)
{
  if (tcp_Sock_size > 0 && tcp_Sock_size < sizeof(_tcp_Socket))
  {
    (*_printf) ("Watt-32 development error: sizeof(_tcp_Socket) in "
                "<tcp.h> too small. %d bytes needed\n", SIZEOF(_tcp_Socket));
    exit (-1);
  }
  if (udp_Sock_size > 0 && udp_Sock_size < sizeof(_udp_Socket))
  {
    (*_printf) ("Watt-32 development error: sizeof(_udp_Socket) in "
                "<tcp.h> too small. %d bytes needed\n", SIZEOF(_udp_Socket));
    exit (-1);
  }
}
#endif


/*
 * Return error text for watt_sock_init() result.
 */
const char * sock_init_err (int rc)
{
  enum eth_init_result rc2 = rc;

  switch (rc2)
  {
    case WERR_ILL_DOSX:
         return _LANG ("Illegal DOS-extender and library combination");

    case WERR_NO_MEM:
         return _LANG ("No memory for buffers");

    case WERR_PKT_ERROR:
         return _LANG ("Error in PKTDRVR interface");

    case WERR_NO_DRIVER:
         return _LANG ("No PKTDRVR found");

    case WERR_BOOTP_FAIL:
         return _LANG ("BOOTP protocol failed");

    case WERR_DHCP_FAIL:
         return _LANG ("DHCP protocol failed");

    case WERR_RARP_FAIL:
         return _LANG ("RARP protocol failed");

    case WERR_NO_IPADDR:
         return _LANG ("Failed to get an IP-address");

    case WERR_PPPOE_DISC:
         return _LANG ("Timeout in PPPoE discovery");

    case WERR_NO_ERROR:
         break;
  }
  return _LANG ("No error");
}


/**
 * The main initialisation routine.
 * Called only once (during program startup).
 * sock_init() is not a macro in <tcp.h>.
 */
static BOOL sock_init_called = FALSE;

int watt_sock_init (size_t tcp_Sock_size, size_t udp_Sock_size)
{
  static int rc = 0;

  if (sock_init_called)  /* return previous result */
     return (rc);

  sock_init_called = TRUE;

#if defined(USE_CRTDBG) || defined(USE_FORTIFY)
  memdbg_init();
#endif

#if defined(WIN32)
  /* Nothing to do here */
#elif defined(__HIGHC__)
  if (_mwenv != PL_ENV)
  {
    outsnl (_LANG("\7Only Pharlap DOS extender supported"));
    return do_exit (rc = WERR_ILL_DOSX);
  }
#elif defined(WATCOM386)
  if (!dpmi_init())
     return do_exit (rc = WERR_ILL_DOSX);
#endif

#if (DOSX & POWERPAK)
  if (!powerpak_init())
  {
    puts ("\7PowerPak initialisation failed");
    return do_exit (rc = WERR_ILL_DOSX);
  }
#endif

#if defined(USE_DEBUG)
  check_sock_sizes (tcp_Sock_size, udp_Sock_size);
#else
  ARGSUSED (tcp_Sock_size);
  ARGSUSED (udp_Sock_size);
#endif

#if defined(USE_DEBUG) && defined(WATT32_DOS_DLL)
  _printf = import_export.printf;
#endif


  /* DOSX: Set DOS far-ptr, get CPU type, use BSWAP
   *       instruction on 486+CPUs, setup DOS transfer buffer.
   * ALL:  Find DOS/Win-version, init timers etc.
   */
  init_misc();

#if !defined(MAKE_TSR)
  setup_sig_handlers();
#endif

  /* Init PKTDRVR, get ether-addr, set config-hook for
   * parsing TCP-values
   */
  rc = tcp_init();

  if (rc && !survive_eth)  /* if survive we pretend all is ok and go on */
     return do_exit (rc);  /* else failed */

  _watt_is_init = TRUE;
  start_time    = set_timeout (0);
  start_day     = get_day_num();


  /* Setup the ARP daemon, GvB 2002-09
   */
  _arp_init();

  /* Prepare to parse WATTCP.CFG config options related
   * to DHCP, SYSLOG, BIND, TFTP, ECHO/DISCARD clients, PPPoE and IPv6.
   */
#if defined(USE_DHCP)
  DHCP_init();
#endif

#if defined(USE_BSD_API)
  syslog_init();
#endif

#if defined (USE_BIND)
  res_init0();
#endif

#if defined(USE_TFTP)
  tftp_init();
#endif

#if defined(USE_ECHO_DISC)
  echo_discard_init();
#endif

#if defined(USE_PPPOE)
  pppoe_init();
#endif

#if defined(USE_DYNIP_CLI)
  dynip_init();
#endif

#if defined(USE_IPV6)
  _ip6_init();
#endif

#if defined(USE_IDNA)
  iconv_init (0);
#endif

  old_break = tcp_cbreak (0x10);

  /* The only atexit() handler
   */
  atexit ((void(*)(void))sock_exit);

#if !defined(MAKE_TSR)
  RUNDOWN_ADD (restore_sig_handlers, 3);
#endif

  if (_watt_no_config && !_watt_user_config_fn)
  {
    if (!my_ip_addr)
    {
#if defined(USE_BOOTP)
      _bootp_on = TRUE;   /* if no fixed IP, try BOOTP/DHCP/RARP */
#endif
#if defined(USE_DHCP)
      _dhcp_on = TRUE;
#endif
#if defined(USE_RARP)
      _rarp_on = TRUE;
#endif
    }
    else if (debug_on)
      outsnl (_LANG("Fixed IP configuration."));
  }

  else if (!tcp_config(NULL))
  {
#if defined(USE_BOOTP)
    _bootp_on = TRUE;     /* if no config file try BOOTP/DHCP/RARP */
#endif
#if defined(USE_DHCP)
    _dhcp_on = TRUE;
#endif
#if defined(USE_RARP)
    _rarp_on = TRUE;
#endif
  }

#if defined(USE_DEBUG)
  if (_dbugxmit)          /* if dbug_init() called */
  {
    dbug_open();
#if defined(USE_BSD_API)
    _sock_dbug_open();
#endif
  }
#endif


#if defined(USE_DHCP)
  /*
   * If we read previously configured values okay, skip the below booting.
   */
  if (_dhcp_on && DHCP_read_config())
  {
    _dhcp_on  = FALSE;
    _bootp_on = FALSE;
    _rarp_on  = FALSE;
  }
#endif

  /* Test and fix configured values
   */
  tcp_post_init();

  if (_bootp_on || _dhcp_on || _rarp_on)
  {
    rc = tcp_do_boot (_bootp_on, _dhcp_on, _rarp_on);
    if (rc && !survive_eth)
       return do_exit (rc);
  }


#if defined(USE_PPPOE)
  if (!pppoe_start())
  {
    outsnl (_LANG("Timeout waiting for PADS/PADO"));
    rc = WERR_PPPOE_DISC;
    if (!survive_eth)
       return do_exit (rc);
  }
#endif

  if (!my_ip_addr)   /* all boot attempts failed */
  {
    outsnl (_LANG("All attempt (config-file/DHCP/BOOTP/RARP) to get "
                  "a IP-address failed.\r\n"
                  "Check your configuration."));

    /** \todo Use auto-configuration and set 'my_ip_addr' to 169.254/16
     */
    rc = WERR_NO_IPADDR;
    if (!survive_eth)
       return do_exit (rc);
  }

  if (_pkt_inf)  /* Warn only if we have a pktdrvr/WinPcap instance */
  {
#if defined(WIN32)
    if (my_ip_addr && !pkt_check_address(my_ip_addr))
       (*_printf) ("Warning: using same IP-address (%s) as Winsock.\n\7",
                   _inet_ntoa(NULL,my_ip_addr));
#endif

#if defined(USE_DEBUG)
    if (sin_mask == 0)
       outsnl ("Warning: \"NETMASK\" is 0.0.0.0 !");

    if (!_arp_check_gateways())
       outsnl ("Warning: multiple or no default gateway!");
#endif
  }

  if (dynamic_host && !reverse_lookup_myip())
     outsnl (_LANG("Cannot reverse resolve local IP-address. "
                   "Set \"DYNAMIC_HOST\" = 0."));

#if defined(USE_MULTICAST)
  if (_multicast_on)
  {
    /* all multicast level 2 systems must join at startup
     */
    join_mcast_group (MCAST_ALL_SYST);
  }
#endif

#if defined(USE_FRAGMENTS)
  /*
   * Add a daemon to check for IP-fragment time-outs.
   */
  addwattcpd (chk_timeout_frags);
#endif

  /* if "ICMP_MASK_REQ = 1" in wattcp.cfg, send an ICMP_MASK_REQ.
   */
  if (_do_mask_req)
     icmp_send_mask_req();

#if defined(USE_TFTP)
  /*
   * If application supplied a tftp_writer hook, try to load
   * the specified BOOT-file.
   */
  if (tftp_writer)
     tftp_boot_load();
#endif

#if defined(USE_ECHO_DISC)
  echo_discard_start();       /* start echo/discard services */
#endif

#if defined(USE_DYNIP_CLI)
  dynip_exec();
#endif

  return (rc);
}

/*
 * Old compatibility (if user didn't include <tcp.h>
 */
#undef sock_init
int sock_init (void)
{
  return watt_sock_init (0, 0);
}


/**
 * Our only atexit() handler.
 * Called only once during normal program shutdown.
 * Exception handlers exit differently (see except_handler() above)
 */
void MS_CDECL sock_exit (void)
{
  if (_watt_is_init || sock_init_called)
  {
    _watt_is_init = sock_init_called = FALSE;

    if (!_watt_fatal_error)
       tcp_shutdown();

    /* Closed the PKTDRVR etc. No effect if already done.
     */
    rundown_run();
  }
}


/**
 * Exit handler for unhandled signals.
 */
void sock_sig_exit (const char *msg, int sig)
{
#if defined(SIGBREAK)
  if (sig == SIGBREAK) /* should only happen for Watcom (from signal.c) */
     sig = SIGINT;
#endif

  if (sig == SIGINT)
     _watt_cbroke++;

  sock_exit();   /* calls registered rundown functions */

  if (msg)
     outsnl (msg);

#if (DOSX)  /* don't pull in _streams[] for SMALL/LARGE model */
  fflush (stdout);
  fflush (stderr);
#endif


#if defined(__HIGHC__)
 /*
  * If "#pragma on(check_stack)" is in effect somewhere, we're sure
  * to crash in an atexit() function. It's not nice, but bypass them
  * by calling _exit(). Application should hook SIGINT itself to clean
  * itself up. Same applies for djgpp apps.
  */
  sig = 0;  /* calls exit() */

#elif defined(__DJGPP__)
 /*
  * Some error (in Watt-32 or djgpp's crt?) causes the INT 1B
  * vector not always to be restored when running with CWSDPMI. This
  * causes a page fault in CWSDPMI. Not nice, but circumvent this by
  * calling _exit(). Not sure if toggling the exceptions will help, so
  * leave them alone. NTVDM doesn't seem to mind we calling _exit()...
  */
/* __djgpp_exception_toggle(); */
  sig = 0;

#elif !defined(__WATCOMC__)
  sig = 0;
#endif

#if (defined(__BORLANDC__) || defined(__TURBOC__)) && !defined(__FLAT__)
  _stklen = 0x1000;   /* avoid stack-overflow during exit() */
#endif

#if !defined(__POCC__)
  if (sig == SIGINT)
    _exit (-1);
  else
#endif
    exit (-1);
}


/*
 * Make sure user links the correct C-libs
 */
#if defined(WATCOM386)
  #if defined(__SW_3R)
    #pragma library ("clib3r.lib");
    #pragma library ("math387r.lib");

  #else /* __SW_3S */
    #pragma library ("clib3s.lib");
    #pragma library ("math387s.lib");
  #endif

#elif defined(_MSC_VER) && (_MSC_VER >= 1200) && 0
  #if defined(_DEBUG)
    #pragma comment (library, "wattcpvc_imp_d.lib")
  #else
    #pragma comment (library, "wattcpvc_imp.lib")
  #endif

#elif defined(_MSC_VER) && (_MSC_VER >= 700) && 0
  #if defined(__SMALL__)
    #pragma comment (library, "wattcpMS.lib")
  #elif defined(__LARGE__)
    #pragma comment (library, "wattcpML.lib")
  #elif defined(__386__)
    #pragma comment (library, "wattcpMF.lib")
  #endif

#elif defined(__DMC__) && 0
  #if defined(WIN32)
    #pragma comment (lib, "wattcpd_imp.lib")
  #elif defined(__SMALL16__)
    #pragma comment (lib, "wattcpDS.lib")
  #elif defined(__LARGE__)
    #pragma comment (lib, "wattcpDL.lib")
  #elif defined(DMC386)
    #pragma comment (lib, "wattcpDF.lib")
  #endif
#endif

