/*!\file pcconfig.h
 */
#ifndef _w32_PCCONFIG_H
#define _w32_PCCONFIG_H

#if !defined (USE_BUFFERED_IO)
#include <fcntl.h> /* Defines O_RDONLY and O_BINARY, needed by FOPEN macro */
#endif

#ifndef __WATT_TCP_H
/**
 * Convert 'arg_func' below to this type.
 */
enum config_tab_types {
     ARG_ATOI,            /**< convert to int */
     ARG_ATOB,            /**< convert to 8-bit byte */
     ARG_ATOW,            /**< convert to 16-bit word */
     ARG_ATOIP,           /**< convert to ip-address (host order) */
     ARG_ATOX_B,          /**< convert to hex-byte */
     ARG_ATOX_W,          /**< convert to hex-word */
     ARG_ATOX_D,          /**< convert to hex-dword */
     ARG_STRDUP,          /**< duplicate string value */
     ARG_STRCPY,          /**< copy string value */
     ARG_RESOLVE,         /**< resolve host to IPv4-address */
     ARG_FUNC             /**< call convertion function */
   };

/*!\struct config_table
 */
struct config_table {
       const char            *keyword;
       enum config_tab_types  type;
       void                  *arg_func;
     };
#endif

#define sock_inactive      NAMESPACE (sock_inactive)
#define sock_data_timeout  NAMESPACE (sock_data_timeout)
#define sock_delay         NAMESPACE (sock_delay)
#define debug_on           NAMESPACE (debug_on)
#define ctrace_on          NAMESPACE (ctrace_on)
#define multihomes         NAMESPACE (multihomes)
#define usr_init           NAMESPACE (usr_init)
#define usr_post_init      NAMESPACE (usr_post_init)
#define cookies            NAMESPACE (cookies)
#define last_cookie        NAMESPACE (last_cookie)
#define dynamic_host       NAMESPACE (dynamic_host)
#define print_hook         NAMESPACE (print_hook)
#define get_argv0          NAMESPACE (get_argv0)
#define parse_config_table NAMESPACE (parse_config_table)
#define ExpandVarStr       NAMESPACE (ExpandVarStr)

W32_DATA int  sock_inactive;
W32_DATA int  sock_data_timeout;
W32_DATA int  sock_delay;
W32_DATA WORD multihomes;

W32_DATA void (*print_hook)   (const char *);
W32_DATA void (*usr_init)     (const char *, const char*);
W32_DATA void (*usr_post_init)(void);

W32_DATA DWORD cookies [MAX_COOKIES];
W32_DATA WORD  last_cookie;
W32_DATA int   debug_on;
W32_DATA int   ctrace_on;
W32_DATA BOOL  dynamic_host;

extern const struct config_table *watt_init_cfg;

extern const char *get_argv0  (void);
extern void       _add_server (WORD *counter, WORD max, DWORD *array, DWORD value);
extern int         netdb_init (void);
extern void        netdb_warn (const char *fname);

W32_FUNC int  tcp_config_name (char *name, int max);
W32_FUNC long tcp_config      (const char *path);
W32_FUNC void tcp_set_debug_state (WORD x);

W32_FUNC int parse_config_table (const struct config_table *tab,
                                 const char *section,
                                 const char *name,
                                 const char *value);

W32_FUNC const char *ExpandVarStr (const char *str);

/*
 * Using buffered I/O speeds up reading config-file, but uses more data/code.
 * Non-DOSX targets where memory is tight doesn't "#define USE_BUFFERED_IO"
 * by default (see config.h)
 */
#if defined (USE_BUFFERED_IO)
  typedef FILE*        WFILE;
  #define FOPEN(f,n)   ( f = fopen (n, "rb"), (f != NULL) )
  #define FREAD(p,f)   fread ((char*)(p), 1, 1, f)
  #define FCLOSE(f)    fclose (f)
#else
  typedef int          WFILE;
  #define FOPEN(f,n)   ( f = open (n, O_RDONLY | O_BINARY), (f != -1) )
  #define FREAD(p,f)   read (f, (char*)(p), 1)
  #define FCLOSE(f)    close (f)
#endif

extern long tcp_parse_file (WFILE f, const struct config_table *cfg);

extern void tcp_inject_config (
            const struct config_table *cfg,
            const char                *key,
            const char                *value);

#ifndef __WATT_TCP_H
typedef long (*WattUserConfigFunc) (int pass, const struct config_table *cfg);
#endif

/*
 * Really is in sock_ini.c, but we need the definition of above
 * 'struct config_table'
 */
extern   WattUserConfigFunc _watt_user_config_fn;
W32_FUNC WattUserConfigFunc _watt_user_config (WattUserConfigFunc fn);

#endif

