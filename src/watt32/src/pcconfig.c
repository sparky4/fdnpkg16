/*!\file pcconfig.c
 *
 * WatTCP config file handling.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "copyrigh.h"
#include "wattcp.h"
#include "strings.h"
#include "misc.h"
#include "timer.h"
#include "language.h"
#include "udp_dom.h"
#include "netaddr.h"
#include "bsdname.h"
#include "pcqueue.h"
#include "pcdbug.h"
#include "pcsed.h"
#include "pcpkt.h"
#include "pctcp.h"
#include "pcarp.h"
#include "pcrarp.h"
#include "pcdhcp.h"
#include "pcbootp.h"
#include "pcicmp.h"
#include "sock_ini.h"
#include "ip4_frag.h"
#include "ip4_out.h"
#include "ip6_out.h"
#include "loopback.h"
#include "get_xby.h"
#include "printk.h"
#include "pcconfig.h"

#if defined(USE_IPV6)
#include "pcicmp6.h"
#include "teredo64.h"
#endif

#if (DOSX & PHARLAP)
  #include <hw386.h>
#endif

#if defined(__BORLANDC__)
  #pragma warn -pro
#endif


int  debug_on          = 0;    /* general debug level */
int  ctrace_on         = 0;    /* tracing; on/off (High-C only) */
int  sock_delay        = 30;
int  sock_inactive     = 0;    /* defaults to forever */
int  sock_data_timeout = 0;    /* timeout sending no data (forever) */
WORD multihomes        = 0;    /* We have more than 1 IP-addresses */
BOOL dynamic_host      = 0;    /* reverse resolve assigned IP to true FQDN */

DWORD cookies [MAX_COOKIES];
WORD  last_cookie = 0;

static void deprecated_key    (const char *old_key, const char *new_key);
static void keyword_not_found (const char *key, const char *value);

void (*print_hook) (const char*) = NULL;
void (*usr_init)   (const char*, const char*) = keyword_not_found;

/** Hook to a function called after we're initialised.
 */
void (*usr_post_init) (void) = NULL;

static char *config_name  = "WATTCP.CFG";  /* name of config file */
static char *current_file = NULL;
static UINT  current_line = 0;
static BOOL  got_eol      = FALSE;  /* got end-of-line in current_line */

static const char *environ_names[] = {
                  "WATTCP.CFG",    /* pointer to config-file */
                  "WATTCP_CFG"     /* ditto, for bash */
                };

enum ParseMode {
     ModeKeyword = 0,
     ModeValue,
     ModeComment
   };

#if defined(USE_DEBUG)
  #define CONFIG_DBG_MSG(lvl, args) \
          do { \
            if (debug_on >= lvl) { \
               UINT line = got_eol ? current_line : current_line+1; \
               if (current_file) \
                    (*_printf) ("%s (%u): ", current_file, line); \
               else (*_printf) ("CONFIG: "); \
               (*_printf) args; \
               fflush (stdout); \
            } \
          } while (0)

  #define RANGE_CHECK(val,low,high) \
          do { \
            if (((val) < (low)) || ((val) > (high))) \
               CONFIG_DBG_MSG (0, ("Value %ld exceedes range [%d - %d]\n", \
                               val, low, high)); \
          } while (0)
#else
  #define CONFIG_DBG_MSG(lvl, args)  ((void)0)
  #define RANGE_CHECK(val,low,high)  ((void)0)
#endif


/*
 * Set 'debug_on' if not already set before tcp_config() called.
 */
static void set_debug_on (const char *value)
{
  if (debug_on == 0)
     debug_on = ATOI (value);
}

/**
 * Add an "ether, IPv4-address" pair to the static ARP-cache.
 * Add an "ether, IPv6-address" pair to the static ND-cache.
 */
static void set_ethip (const char *value)
{
  eth_address eth;
  const char *trailer = _inet_atoeth (value, &eth);
  DWORD       ip4;

  if (!trailer)
     return;

  ip4 = _inet_addr (trailer);

  /* Don't add our MAC/IP-address
   */
  if (!memcmp (&eth, _eth_real_addr, sizeof(eth)))
     return;

  if (!ip4)
  {
#if defined(USE_IPV6)
    const void *ip6 = _inet6_addr (trailer);
    if (ip6)
       icmp6_ncache_insert_fix (ip6, &eth);
#endif
    return;
  }

  /* Add IPv4-addr to ARP-cache.
   */
  _arp_add_cache (ip4, (const eth_address*)&eth, FALSE);
}

void _add_server (WORD *counter, WORD max, DWORD *array, DWORD value)
{
  int i, duplicate = 0;

  if (value && *counter < max)
  {
    for (i = 0; i < *counter; i++)
        if (array[i] == value)
           duplicate = 1;

    if (!duplicate)
       array [(*counter)++] = value;
  }
}

/**
 * Return a string with a environment variable expanded (only one).
 *
 * E.g. if environment variable ETC is "c:\network\watt\bin",
 * "$(ETC)\hosts" becomes "c:\network\watt\bin\hosts"
 * If ETC isn't defined, "$(ETC)\hosts" becomes "\hosts".

 * \todo support several $(x) in one line and malloc the result.
 */
const char *ExpandVarStr (const char *str)
{
  static char buf [MAX_PATHLEN+MAX_VALUELEN+1];
  char   env [30];
  const  char *e, *p  = strstr (str, "$(");
  const  char *dollar = p;
  size_t i;

  if (!p || strlen(p) < 4 || !strchr(p+3,')'))  /* minimum "$(x)" */
     return (str);

  for (i = 0, p += 2; i < sizeof(env)-1; i++)
  {
    if (*p == ')')
       break;
    env[i] = *p++;
  }
  env[i] = '\0';
  strupr (env);
  e = getenv (env);
  if (!e)
  {
    e = env;
    env[0] = '\0';
  }

  i = dollar - str;
  strncpy (buf, str, i);
  buf[i] = '\0';
  strncat (buf, e, sizeof(buf)-1-i);
  return strcat (buf, p+1);
}

/**
 * This function eventually gets called if no keyword was matched.
 * Prints a warning (file+line) in debug-mode >= 2.
 */
static void keyword_not_found (const char *key, const char *value)
{
  CONFIG_DBG_MSG (2, ("unhandled key/value: \"%s = %s\"\n", key, value));
  ARGSUSED (key);
  ARGSUSED (value);
}

/**
 * Parse and store ARG_ATOX_B, ARG_ATOX_W or ARG_ATOI value.
 * Accept "0x" or "x" prefix for ATOX types only. Accept ATOX values
 * without a prefix too.
 */
static BOOL set_value (BOOL is_hex, const char *value, void *arg, int size)
{
  long  val = 0;
  BOOL  ok = FALSE;
  const char *s = value;

#if (DOSX)
  if (is_hex)
     ok = (sscanf(s,"0x%lX",&val) == 1 || sscanf(s,"x%lX",&val) == 1);
  if (!ok)
     ok = (sscanf(s,"%ld",&val) == 1);
#else
  if (is_hex)
  {
    int ch, len = 0;

    if (s[0] == '0' && s[1] == 'x' && strlen(s) >= 4)
    {
      len += 2;
      s += 2;
    }
    else if (s[0] == 'x' && strlen(s) >= 3)
    {
      len++;
      s++;
    }

    ch = toupper (*s);
    if (len > 0 && strchr(hex_chars_upper,ch))
    {
      val = atox (s-2);
      if (strlen(s) >= 4)
         val = (val << 8) + atox (s);
      if (strlen(s) >= 6)
         val = (val << 8) + atox (s+2);
      if (strlen(s) >= 8)
         val = (val << 8) + atox (s+4);
      ok = TRUE;
    }
  }
  if (!ok)
  {
    val = ATOI (value);
    ok = TRUE;  /* could be unconvertable */
  }
#endif

  if (!ok)
  {
    CONFIG_DBG_MSG (0, ("failed to match `%s' as %sdecimal.\n",
                    s, is_hex ? "hex or " : ""));
    return (FALSE);
  }

  CONFIG_DBG_MSG (4, ("got value %ld/%#lx from `%s'\n",
                  val, val, value));
  switch (size)
  {
    case 1:
         RANGE_CHECK (val, 0, UCHAR_MAX);
         *(BYTE*)arg = (BYTE) min (val, UCHAR_MAX);
         break;
    case 2:
#if (DOSX == 0)
         RANGE_CHECK (val, INT_MIN, INT_MAX);
         *(int*)arg = (int) min (max(val,INT_MIN), INT_MAX);
#else
         RANGE_CHECK (val, 0, USHRT_MAX);
         *(WORD*)arg = (WORD) min (val, USHRT_MAX);
#endif
         break;
    case 4:
         *(DWORD*)arg = (DWORD)val;
         break;

    default:
#if (DOSX)
        if (!valid_addr((DWORD)arg,sizeof(int)))
        {
          CONFIG_DBG_MSG (0, ("Illegal 'arg' addr %08lX\n", (DWORD)arg));
	  return (FALSE);
        }
#endif
         *(int*)arg = val;
         break;
  }
  return (TRUE);
}

/**
 * Parse the config-table and if a match is found for
 * ('section'+'.'+)'name' either store variable to 'value' or
 * call function with 'value'.
 */
int parse_config_table (const struct config_table *tab,
                        const char *section,
                        const char *name,
                        const char *value)
{
  for ( ; tab && tab->keyword; tab++)
  {
    char  keyword [MAX_NAMELEN], *p;
    DWORD host;
    void *arg;

    if (section)
    {
      if (strlen(section) + strlen(tab->keyword) >= sizeof(keyword))
         continue;
      strcpy (keyword, section);
      strcat (keyword, tab->keyword);   /* "SECTION.KEYWORD" */
    }
    else
      StrLcpy (keyword, tab->keyword, sizeof(keyword));

    if (strcmp(name,keyword))
       continue;

    arg = tab->arg_func;   /* storage or function to call */
    if (!arg)
    {
      CONFIG_DBG_MSG (2, ("No storage for \"%s\", type %d\n",
                      keyword, tab->type));
      return (1);
    }

    switch (tab->type)
    {
      case ARG_ATOI:
           set_value (FALSE, value, arg, sizeof(int));
           break;

      case ARG_ATOB:
           set_value (FALSE, value, arg, sizeof(BYTE));
           break;

      case ARG_ATOX_B:
           set_value (TRUE, value, arg, sizeof(BYTE));
           break;

      case ARG_ATOW:
           set_value (FALSE, value, arg, sizeof(WORD));
           break;

      case ARG_ATOX_W:
           set_value (TRUE, value, arg, sizeof(WORD));
           break;

      case ARG_ATOX_D:
           set_value (TRUE, value, arg, sizeof(DWORD));
           break;

      case ARG_ATOIP:
           *(DWORD*)arg = aton (value);
           break;

      case ARG_FUNC:
           (*(void(*)(const char*, int))arg) (value, strlen(value));
           break;

      case ARG_RESOLVE:
           host = resolve (value);
           if (!host && !isaddr(value))
           {
             outs (name);
             outs (_LANG(": Cannot resolve \""));
             outs (value);
             outsnl ("\"");
           }
           else
             *(DWORD*)arg = host;
           break;

      case ARG_STRDUP:
           p = strdup (value);
           if (!p)
           {
             outs (_LANG("No memory for \""));
             outs (name);
             outsnl ("\"");
           }
           else
             *(char**)arg = p;
           break;

      case ARG_STRCPY:
           StrLcpy ((char*)arg, value, MAX_VALUELEN);
           break;

      default:
#if defined(USE_DEBUG)
           fprintf (stderr, "Something wrong in parse_config_table().\n"
                    "Section %s; `%s' = `%s'\n", section, name, value);
           exit (-1);
#endif
           break;
    }

    TCP_CONSOLE_MSG (3,
       ("ARG_%s, matched `%s' = `%s'\n",
        tab->type == ARG_ATOI    ? "ATOI   " :
        tab->type == ARG_ATOB    ? "ATOB   " :
        tab->type == ARG_ATOW    ? "ATOW   " :
        tab->type == ARG_ATOIP   ? "ATOIP  " :
        tab->type == ARG_ATOX_B  ? "ATOX_B " :
        tab->type == ARG_ATOX_W  ? "ATOX_W " :
        tab->type == ARG_FUNC    ? "FUNC   " :
        tab->type == ARG_RESOLVE ? "RESOLVE" :
        tab->type == ARG_STRDUP  ? "STRDUP " :
        tab->type == ARG_STRCPY  ? "STRCPY " : "??",
        keyword, value));

    return (1);
  }
  return (0);
}

static void set_my_ip (const char *value)
{
  if (!stricmp(value,"bootp"))
       _bootp_on = 1;
  else if (!stricmp(value,"dhcp"))
       _dhcp_on = 1;
  else if (!stricmp(value,"rarp"))
       _rarp_on = 1;
  else my_ip_addr = resolve (value);
}

static void set_hostname (const char *value)
{
  StrLcpy (hostname, value, sizeof(hostname));
}

static void set_gateway (const char *value)
{
  _arp_add_gateway (value, 0L);  /* accept gateip[,subnet[,mask]] */
}

static void set_nameserv (const char *value)
{
  _add_server (&last_nameserver, MAX_NAMESERVERS,
               def_nameservers, resolve(value));
}

static void set_cookie (const char *value)
{
  _add_server (&last_cookie, MAX_COOKIES, cookies, resolve(value));
}

/**
 * Set new ether-address from "ether" value.
 */
static void set_eaddr (const char *value)
{
  eth_address eth;

  if (!_inet_atoeth (value, &eth))
     return;

  if (memcmp (&eth, _eth_real_addr, sizeof(eth)))
  {
    if (!_eth_set_addr(&eth))
       outsnl (_LANG("Cannot set Ether-addr"));
#if defined(USE_RARP)
    else
    {
      WORD  save_to = _rarptimeout;
      DWORD save_ip = my_ip_addr;

      /* We need to debug the RARP messages. It doesn't hurt to
       * call dbug_open() again in sock_ini.c
       */
#if defined(USE_DEBUG)
     if (_dbugxmit)
        dbug_open();
#endif
      _rarptimeout = 2;     /* use only 2 sec timeout */
      my_ip_addr   = 0;
      if (_dorarp() && my_ip_addr == save_ip)
         outsnl (_LANG("Warning: MAC-addr already in use"));
      my_ip_addr   = save_ip;
      _rarptimeout = save_to;
    }
#endif
  }

}

static void set_domain (const char *value)
{
  setdomainname (value, sizeof(defaultdomain)-1);
}

static void depr_set_domain1 (const char *value)
{
  deprecated_key ("DOMAINSLIST", "DOMAIN.SUFFIX");
  setdomainname (value, sizeof(defaultdomain)-1);
}

static void depr_set_domain2 (const char *value)
{
  deprecated_key ("DOMAIN_LIST", "DOMAIN.SUFFIX");
  setdomainname (value, sizeof(defaultdomain)-1);
}

static void do_print (const char *str)
{
  if (print_hook)
       (*print_hook) (str);
  else outsnl (str);
}

static void depr_dns_timeout1 (const char *value)
{
  deprecated_key ("DOMAINTO", "DOMAIN.TIMEOUT");
  dns_timeout = ATOI (value);
}

static void depr_dns_timeout2 (const char *value)
{
  deprecated_key ("DOMAIN_TO", "DOMAIN.TIMEOUT");
  dns_timeout = ATOI (value);
}

static void depr_dns_recurse (const char *value)
{
  deprecated_key ("DOMAIN_RECURSE", "DOMAIN.RECURSE");
  dns_recurse = ATOI (value);
}

/**
 * Open and parse an include file.
 * Syntax: "include = [?]<file>". If '?' prefix is used, the parser
 * will not warn if the file isn't found.
 */
static long do_include_file (const char *value, int len)
{
  const char *p = value;
  long  rc = 0;

  if (*p == '?' && len > 1)
     ++p;

  /* If this is pass 1 of the parsing (ref. pcpkt.c), then return
   */
  if (usr_init == NULL)
  {
    CONFIG_DBG_MSG (2, ("Skipping include file `%s'\n", p));
    return (0);
  }

#ifdef __DJGPP__
  if (_chmod(p, 0) != -1)
#else
  if (access(p, 0) == 0)
#endif
  {
    /* Recursion, but we're reentrant.
     * !!Fix-me: recursion depth should be limited.
     */
    UINT  tmp_line = current_line;
    char *tmp_file = current_file;

    rc = tcp_config (p);

    current_line = tmp_line;
    current_file = tmp_file;
  }
  else if (*value != '?')
  {
    outs (_LANG("\nUnable to open \""));
    outs (p);
    outsnl ("\"");
    rc = 0;
  }
  return (rc);
}

static void deprecated_key (const char *old_key, const char *new_key)
{
#if defined(USE_DEBUG)
  if (current_file && current_line > 0)
     printf ("%s (%u): ", current_file, current_line);
  printf ("Keyword \"%s\" is deprecated. Use \"%s\" instead.\n",
          old_key, new_key);
#endif

  ARGSUSED (old_key);
  ARGSUSED (new_key);
}

/**
 * Return argv[0] as passed to main().
 */
const char *get_argv0 (void)
{
  const char *ret;

#if defined(WIN32)
  static char buf [MAX_PATH];

  if (GetModuleFileNameA(NULL, buf, sizeof(buf)))
       ret = buf;
  else ret = NULL;

#elif (DOSX & PHARLAP)
  static     char buf[MAX_PATHLEN];
  CONFIG_INF cnf;
  UINT       limit;
  CD_DES     descr;
  FARPTR     fp;
  char      *env, *start;

  _dx_config_inf (&cnf, (UCHAR*)&cnf);

  if (_dx_ldt_rd(cnf.c_env_sel,(UCHAR*)&descr))
     return (NULL);

  limit = descr.limit0_15 + ((descr.limit16_19 & 15) << 16);
  env   = malloc (limit);
  if (!env)
     return (NULL);

  start = env;
  FP_SET (fp, 0, cnf.c_env_sel);
  ReadFarMem ((void*)env, fp, limit);

  /* The environment is organised like this:
   * 'PATH=c:\dos;c:\util',0
   *  ..
   * 'COMSPEC=c:\dos\command.com',0,0,0
   * 'program.exe',0
   *
   */
  while (*(env+1))
        env += 1 + strlen (env);
  env += 2;
  StrLcpy (buf, env, sizeof(buf));
  free (start);
  ret = buf;

#elif defined(__DMC__)
  ret = __argv[0];

#elif defined (__DJGPP__)
  #if !defined(WATT32_DOS_DLL)
  extern char **__crt0_argv;
  #endif
  ret = __crt0_argv[0];

#elif defined(_MSC_VER)
  extern char **__argv;
  ret = __argv[0];

#else
  extern char **_argv;   /* Borland, Watcom */
  ret = _argv[0];
#endif

  if (!ret || !ret[0])
     return (NULL);
  return (ret);
}

#if !defined(USE_UDP_ONLY)
static void set_recv_win (const char *value)
{
  DWORD val = ATOL (value);

  if (val < tcp_MaxBufSize)
      val = tcp_MaxBufSize;
  if (val > MAX_WINDOW)
      val = MAX_WINDOW;  /* Window-scaling not yet supported */
  tcp_recv_win = val;
}
#endif



/*
 * Our table of Wattcp "core" values. Other modules have their
 * own tables which are hooked into the chain via `usr_init'.
 * If `name' (left column) isn't found in table below, `usr_init'
 * is called to pass on `name' and `value' to another module or
 * application.
 */
static const struct config_table normal_cfg[] = {
       { "MY_IP",         ARG_FUNC,   (void*)set_my_ip          },
       { "HOSTNAME",      ARG_FUNC,   (void*)set_hostname       },
       { "NETMASK",       ARG_ATOIP,  (void*)&sin_mask          },
       { "GATEWAY",       ARG_FUNC,   (void*)set_gateway        },
       { "NAMESERVER",    ARG_FUNC,   (void*)set_nameserv       },
       { "COOKIE",        ARG_FUNC,   (void*)set_cookie         },
       { "EADDR",         ARG_FUNC,   (void*)set_eaddr          },
       { "ETHIP",         ARG_FUNC,   (void*)set_ethip          },
       { "DEBUG",         ARG_FUNC,   (void*)set_debug_on       },
       { "BOOTP",         ARG_RESOLVE,(void*)&_bootp_host       },
       { "BOOTPTO",       ARG_ATOI,   (void*)&_bootp_timeout    },
       { "BOOTP_TO",      ARG_ATOI,   (void*)&_bootp_timeout    },
       { "SOCKDELAY",     ARG_ATOI,   (void*)&sock_delay        },
       { "MSS",           ARG_ATOI,   (void*)&_mss              },
       { "MTU",           ARG_ATOI,   (void*)&_mtu              },

       { "DOMAIN.SUFFIX", ARG_FUNC,   (void*)set_domain         },
       { "DOMAIN.TIMEOUT",ARG_ATOI,   (void*)&dns_timeout       },
       { "DOMAIN.RECURSE",ARG_ATOI,   (void*)&dns_recurse       },
       { "DOMAIN.IDNA",   ARG_ATOI,   (void*)&dns_do_idna       },
       { "DOMAIN.DO_IPV6",ARG_ATOI,   (void*)&dns_do_ipv6       },
       { "DOMAIN.WINDNS", ARG_ATOX_W, (void*)&dns_windns        },

       /* These are kept for backward compatability. Delete
        * them some day.
        */
       { "DOMAINSLIST",   ARG_FUNC,   (void*)depr_set_domain1   },
       { "DOMAIN_LIST",   ARG_FUNC,   (void*)depr_set_domain2   },
       { "DOMAINTO",      ARG_FUNC,   (void*)depr_dns_timeout1  },
       { "DOMAIN_TO",     ARG_FUNC,   (void*)depr_dns_timeout2  },
       { "DOMAIN_RECURSE",ARG_FUNC,   (void*)depr_dns_recurse   },

       { "MULTIHOMES",    ARG_ATOI,   (void*)&multihomes        },
       { "ICMP_MASK_REQ", ARG_ATOI,   (void*)&_do_mask_req      },
       { "DYNAMIC_HOST",  ARG_ATOI,   (void*)&dynamic_host      },
       { "RAND_LPORT",    ARG_ATOI,   (void*)&use_rand_lport    },
       { "REDIRECTS",     ARG_FUNC,   (void*)icmp_doredirect    },
       { "PRINT",         ARG_FUNC,   (void*)do_print           },
       { "INCLUDE",       ARG_FUNC,   (void*)do_include_file    },
#if defined(USE_PROFILER)
       { "PROFILE.ENABLE",ARG_ATOI,   (void*)&profile_enable    },
       { "PROFILE.FILE",  ARG_STRCPY, (void*)&profile_file      },
#endif
#if defined (USE_LANGUAGE)
       { "LANGUAGE",      ARG_FUNC,   (void*)lang_init          },
#endif
#if defined (USE_BSD_API)
       { "HOSTS",         ARG_FUNC,   (void*)ReadHostsFile      },
       { "SERVICES",      ARG_FUNC,   (void*)ReadServFile       },
       { "PROTOCOLS",     ARG_FUNC,   (void*)ReadProtoFile      },
       { "NETWORKS",      ARG_FUNC,   (void*)ReadNetworksFile   },
       { "NETDB_ALIVE",   ARG_ATOI,   (void*)&netdbCacheLife    },
       { "ETHERS",        ARG_FUNC,   (void*)InitEthersFile     },
#endif
       { "IP.DEF_TTL",    ARG_ATOI,   (void*)&_default_ttl      },
       { "IP.DEF_TOS",    ARG_ATOX_B, (void*)&_default_tos      },
       { "IP.ID_INCR",    ARG_ATOI,   (void*)&_ip4_id_increment },
       { "IP.DONT_FRAG",  ARG_ATOI,   (void*)&_ip4_dont_frag    },
       { "IP.FRAG_REASM", ARG_ATOI,   (void*)&_ip4_frag_reasm   },
       { "IP.LOOPBACK",   ARG_ATOX_W, (void*)&loopback_mode     },
#if !defined(USE_UDP_ONLY)
       { "DATATIMEOUT",         ARG_ATOI, (void*)&sock_data_timeout }, /* EE Aug-99 */
       { "INACTIVE",            ARG_ATOI, (void*)&sock_inactive     },
       { "TCP.NAGLE",           ARG_ATOI, (void*)&tcp_nagle         },
       { "TCP.OPT.TS",          ARG_ATOI, (void*)&tcp_opt_ts        },
       { "TCP.OPT.SACK",        ARG_ATOI, (void*)&tcp_opt_sack      },
       { "TCP.OPT.WSCALE",      ARG_ATOI, (void*)&tcp_opt_wscale    },
       { "TCP.TIMER.OPEN_TO",   ARG_ATOI, (void*)&tcp_OPEN_TO       },
       { "TCP.TIMER.CLOSE_TO",  ARG_ATOI, (void*)&tcp_CLOSE_TO      },
       { "TCP.TIMER.RTO_ADD",   ARG_ATOI, (void*)&tcp_RTO_ADD       },
       { "TCP.TIMER.RTO_BASE",  ARG_ATOI, (void*)&tcp_RTO_BASE      },
       { "TCP.TIMER.RTO_SCALE", ARG_ATOI, (void*)&tcp_RTO_SCALE     },
       { "TCP.TIMER.RESET_TO",  ARG_ATOI, (void*)&tcp_RST_TIME      },
       { "TCP.TIMER.RETRAN_TO", ARG_ATOI, (void*)&tcp_RETRAN_TIME   },
       { "TCP.TIMER.KEEPALIVE", ARG_ATOI, (void*)&tcp_keep_idle     },
       { "TCP.TIMER.KEEPINTVL", ARG_ATOI, (void*)&tcp_keep_intvl    },
       { "TCP.TIMER.MAX_IDLE",  ARG_ATOI, (void*)&tcp_max_idle      },
       { "TCP.TIMER.MAX_VJSA",  ARG_ATOI, (void*)&tcp_MAX_VJSA      },
       { "TCP.TIMER.MAX_VJSD",  ARG_ATOI, (void*)&tcp_MAX_VJSD      },
       { "TCP.MTU_DISCOVERY",   ARG_ATOI, (void*)&mtu_discover      },
       { "TCP.BLACKHOLE_DETECT",ARG_ATOI, (void*)&mtu_blackhole     },
       { "TCP.RECV_WIN",        ARG_FUNC, (void*)set_recv_win       },
#endif
       { NULL, 0, NULL }
     };

/**
 * Used when DEBUG=x is defined in the config file
 * Moved from pctcp.c
 */
void tcp_set_debug_state (WORD x)
{
  debug_on = x;
}

/**
 * Called from config-file parser after "key = value" is found.
 * Take 'value' and possibly expand any $(var) in it.
 * Pass key/value to config_table parser.
 */
static void tcp_inject_config_direct (
            const struct config_table *cfg,
            const char                *key,
            const char                *value)
{
  WATT_ASSERT (key);
  WATT_ASSERT (value);

  if (!key[0])
  {
    CONFIG_DBG_MSG (1, ("empty keyword\n"));
    return;
  }
  if (!value[0])  /* don't pass empty values to the parser */
  {
    CONFIG_DBG_MSG (1, ("keyword `%s' with no value\n", key));
    return;
  }

  value = ExpandVarStr (value);
  if (!parse_config_table(cfg, NULL, key, value) && usr_init)
     (*usr_init) (key, value);
}

/**
 * Callable from a user application to inject config values before
 * the normal WATTCP.CFG is loaded and parsed.
 * See '_watt_user_config' in sock_ini.c.
 */
void tcp_inject_config (const struct config_table *cfg,
                        const char                *key,
                        const char                *value)
{
  char  theKey  [MAX_NAMELEN +1];
  char  theValue[MAX_VALUELEN+1];
  char *save_current_file = current_file;
  UINT  save_current_line = current_line;

  if (!key)
  {
    CONFIG_DBG_MSG (1, ("NULL keyword\n"));
    return;
  }

  strntrimcpy (theKey, key, MAX_NAMELEN);
  theKey[MAX_NAMELEN] = '\0';
  strupr (theKey);

  if (!value)
  {
    CONFIG_DBG_MSG (1, ("keyword `%s' with NULL value\n", theKey));
    return;
  }

  strntrimcpy (theValue, value, MAX_VALUELEN);
  theValue[MAX_VALUELEN] = '\0';

  current_file = NULL;
  current_line = 0;

  tcp_inject_config_direct (cfg, theKey, theValue);

  current_file = save_current_file;
  current_line = save_current_line;
}


/*
 * Read a character:
 *  - if failed (EOF) or read a ^Z, set 'eof' and return '\0'.
 *  - else update 'num_read' counter and return char read.
 */
#define READNEXTCH(ch, f) \
        ((FREAD(&(ch), f) == 1) && (ch != 26) ? (++num_read, ch) : \
          (last_eol = got_eol, got_eol = eof = TRUE, ch = '\0'))

/* Either read a char or retrieve the one "read ahead" before.
 */
#define READCH(ch, next, f) \
        (next ? (ch = next, next = '\0', ch) : READNEXTCH(ch, f))

long tcp_parse_file (WFILE f, const struct config_table *cfg)
{
  char   key  [MAX_NAMELEN+1];
  char   value[MAX_VALUELEN+1];
  char   ch, nextch;
  size_t num;            /* # of char in key/value */
  long   num_read;       /* # of bytes read */
  enum   ParseMode mode;
  BOOL   quotemode;
  BOOL   stripping;
  BOOL   eof;
  BOOL   last_eol = FALSE;
  BOOL   equal_sign;

  num_read = 0;
  nextch   = '\0';
  eof      = FALSE;

  while (!eof)
  {
    *key = *value = '\0';
    mode = ModeKeyword;
    num  = 0;
    got_eol    = FALSE;
    quotemode  = FALSE;
    stripping  = TRUE;
    equal_sign = FALSE;

    for (;;)
    {
      switch (READCH(ch, nextch, f))
      {
        case '\0':
             if (!last_eol && num)
                CONFIG_DBG_MSG (0, ("Missing line-termination "
                                "could break old WatTcp programs.\n"));
             break;

        case '\r':
             if (READNEXTCH(nextch, f) == '\n')
                nextch = '\0';
             /* Fall through */

        case '\n':
             got_eol = TRUE;
             ch = '\0';  /* Do not add */
             break;

        case '#':
        case ';':
             if (!quotemode)
                mode = ModeComment;
             break;

        case '=':
             if (!quotemode && mode == ModeKeyword)
             {
               mode = ModeValue;
               num  = 0;
               ch   = '\0'; /* Don't add */
               stripping  = TRUE;
               equal_sign = TRUE;
             }
             break;

        case '\"':
             if (mode != ModeValue)
             {
               ch = '\0';
               break;
             }

             /* Double quotes may be inserted by doubling them (VB-style)
              * Ex.: SETTING_1 = "This is a ""quoted"" string."
              *      SETTING_2 = This_is_a_""quoted""_string.
              */
              if (READNEXTCH(nextch, f) == '\"')
              {
                nextch = '\0'; /* Ignore next, but keep this 'ch' */
              }
              else
              {
                quotemode ^= 1;
                stripping = FALSE;
                ch = '\0'; /* Do not add */
              }
              break;

        case ' ':
        case '\t':
             if (stripping)
                ch = '\0';
             break;
      }   /* end switch */

      if (ch)
      {
        switch (mode)
        {
          case ModeKeyword:
               stripping = FALSE;
               if (num <= sizeof(key)-2)
               {
                 key[num++] = toupper (ch);
                 key[num]   = '\0';
               }
               break;
          case ModeValue:
               stripping = FALSE;
               if (num <= sizeof(value)-2)
               {
                 value[num++] = ch;
                 value[num]   = '\0';
               }
               break;
          default:      /* squelch gcc warning */
               break;
        }
      }

      if (got_eol)
      {
        if (!eof)
           current_line++;
        if (quotemode)
           CONFIG_DBG_MSG (0, ("Missing right \" in quoted string: `%s'\n",
                           value));
        strrtrim (key);
        strrtrim (value);
        if (key[0] && value[0])
           tcp_inject_config_direct (cfg, key, value);
        else if (!key[0] && !value[0] && equal_sign)
           CONFIG_DBG_MSG (0, ("Both keyword and value missing\n"));
        break;
      }
    }   /* end for (;;) */
  }     /* end while (!eof) */

  return (num_read);
}

/*
 * Public only because of tcpinfo program.
 */
int tcp_config_name (char *name, int max)
{
  char *path, *temp;
  int   i;

  for (i = 0; i < DIM(environ_names); i++)
  {
    path = getenv (environ_names[i]);
    if (path)
    {
      path = StrLcpy (name, path, max-2);
      break;
    }
  }

  if (path)
  {
    temp = strrchr (path, '\0');
    if (temp[-1] != '\\' && temp[-1] != '/')
    {
      *temp++ = '\\';
      *temp = '\0';
    }
  }
#ifdef __DJGPP__
  else if (_chmod(config_name,0) != -1)
#else
  else if (access(config_name,0) == 0)
#endif
  { /* found in current directory */
    strcpy (name, ".\\");
    path = name;
  }
  else if (_watt_os_ver >= 0x300)  /* not found, get path from argv[0] */
  {
    const char *argv0 = get_argv0();

    if (!argv0 || !argv0[0])
       return (0);

    StrLcpy (name, argv0, max);
    strreplace ('/', '\\', name);

    /* If path == "x:", extract path.
     * temp -> last '\\' in path.
     */
    path = (isalpha(name[0]) && name[1] == ':') ? name+2 : name;
    temp = strrchr (path, '\\');
    if (!temp)
       temp = (char*)path;
    temp++;
    *temp = '\0';             /* 'name' = path of program ("x:\path\") */
  }

  i = max - strlen (name) - 1;
  if (i < 0)
     return (0);

  strncat (name, config_name, i); /* 'name' = "x:\path\wattcp.cfg" */
  strlwr (name);
  return (1);
}

/*
 * Hack for pcpkt.c/winpcap.c: Bypass parsing of "normal_cfg".
 * tcp_config() is called in pcpkt.c/winpcap.c with another config-table.
 */
const struct config_table *watt_init_cfg = normal_cfg;

long tcp_config (const char *path)
{
  char  name[MAX_PATHLEN] = { 0 };
  char *fname = config_name;
  WFILE file;
  long  len;
  int   pass = (usr_init == NULL) ? 1 : 2;

  if (_watt_no_config && !path)
  {
    if (!_watt_user_config_fn)
       return (0);
    current_file = NULL;
    current_line = 0;
    return (*_watt_user_config_fn) (pass, watt_init_cfg);
  }

  if (!path)
  {
    if (!tcp_config_name(name, sizeof(name)))
       goto not_found;
    fname = name;
  }
  else
  {
    fname = name;
    StrLcpy (name, path, sizeof(name));
#ifdef __DJGPP__
    if (_chmod(fname,0) == -1)
#else
    if (access(fname,0) != 0)
#endif
       goto not_found;
  }

  if (!FOPEN(file,fname))  /* shouldn't happen */
     goto not_found;

  current_file = name;
  current_line = 0;

  TCP_CONSOLE_MSG (2, ("Parsing `%s' (pass %d)\n", fname, pass));

  len = tcp_parse_file (file, watt_init_cfg);

  FCLOSE (file);
  return (len);

not_found:

  /* Warn unless parsing from pcpkt.c/winpcap.c.
   */
  if (watt_init_cfg == normal_cfg)
  {
    outs (fname);
    outsnl (_LANG(" not found"));
  }
  return (0);
}

#if defined(USE_BSD_API)
/**
 * Called from most <netdb.h> functions in case watt_sock_init()
 * wasn't called first to initialise things.
 */
int netdb_init (void)
{
  int rc, save = _watt_do_exit;

  _watt_do_exit = 0;    /* don't make watt_sock_init() call exit() */
  rc = watt_sock_init (0, 0);
  _watt_do_exit = save;
  return (rc == 0);
}

void netdb_warn (const char *fname)
{
  fprintf (stderr, "Warning: `%s' not found\n", fname);
}
#endif

/*
 * A test program.
 */

#if defined(TEST_PROG)

#include "getopt.h"

const char *cfg_file;

static void not_found2 (const char *key, const char *value)
{
  size_t len = strlen (value);
  DWORD  ret = 0;

  printf ("Unmatched: key `%s', value `%s', len %lu  %s\n",
          key, value, len, len >= MAX_VALUELEN-1 ? "(truncated)" : "");

  if (!stricmp("HEX_BYTE",key))
     set_value (TRUE, value, &ret, sizeof(BYTE));

  if (!stricmp("HEX_WORD",key))
     set_value (TRUE, value, &ret, sizeof(WORD));

  if (!stricmp("HEX_DWORD",key))
     set_value (TRUE, value, &ret, sizeof(DWORD));
}

static long my_injector (int pass, const struct config_table *cfg)
{
  printf ("my_injector(), pass %d\n", pass);
  tcp_inject_config (cfg, "My_IP", "192.168.0.98");
  tcp_inject_config (cfg, "NetMask", "255.255.255.0");

  /* doing the file again doesn't work
   */
  tcp_inject_config (cfg, "include", cfg_file);
  return (1);
}

void Usage (void)
{
  puts ("Usage: pcconfig [-d debug-level] [config-file]\n"
        "if config-file is omitted, the default "
        "$(WATTCP.CFG)\\WATTCP.CFG is used.");
  exit (0);
}

int main (int argc, char **argv)
{
  int  ch, cfg_len;
  long file_len = 0L;

  while ((ch = getopt(argc, argv, "h?d:")) != EOF)
    switch (ch)
    {
      case 'd':
           debug_on = ATOI (optarg);
           break;
      case '?':
      case 'h':
      default:
           Usage();
           break;
    }

  argc -= optind;
  argv += optind;
  cfg_file = NULL;    /* the default one */

  if (*argv)
  {
    FILE *cfg;

    cfg_file = *argv;
    cfg = fopen (cfg_file, "rb");
    if (cfg)
    {
      file_len = filelength (fileno(cfg));
      fclose (cfg);
    }
  }

  usr_init = not_found2;
  cfg_len  = tcp_config (cfg_file);

  if (file_len)
     printf ("filesize %ld, %d bytes parsed\n", file_len, cfg_len);

  printf ("\nTest the config injector:\n");
  _watt_user_config (my_injector);
  _watt_no_config = 1;
  tcp_config (NULL);
  return (0);
}
#endif
