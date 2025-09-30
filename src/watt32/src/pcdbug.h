/*!\file pcdbug.h
 */
#ifndef _w32_PCDBUG_H
#define _w32_PCDBUG_H

typedef void (*DebugProc) (const void *sock, const in_Header *ip,
                           const char *file, unsigned line);

W32_DATA DebugProc _dbugxmit, _dbugrecv;

extern const char *tcpStateName (UINT state);

extern BOOL dbg_mode_all, dbg_print_stat, dbg_dns_details;

W32_FUNC void  dbug_init (void);
extern   void  dbug_open (void);
extern   FILE *dbug_file (void);
extern   int   dbug_write (const char *);

extern int MS_CDECL dbug_printf (const char *fmt, ...) ATTR_PRINTF (1, 2);

#if defined(USE_PPPOE)
  extern const char *pppoe_get_code (WORD code);
#endif

/*
 * Send Rx/Tx packet to debug-file.
 * 'nw_pkt' must point to network layer packet.
 */
#if defined(USE_DEBUG)
  #define DEBUG_RX(sock, nw_pkt)                            \
          do {                                              \
            if (_dbugrecv)                                  \
              (*_dbugrecv) (sock, (const in_Header*)nw_pkt, \
                            __FILE__, __LINE__);            \
          } while (0)

  #define DEBUG_TX(sock, nw_pkt)                            \
          do {                                              \
            if (_dbugxmit)                                  \
              (*_dbugxmit) (sock, (const in_Header*)nw_pkt, \
                           __FILE__, __LINE__);             \
          } while (0)

  /* Generic trace to wattcp.dbg file
   */
  #define TCP_TRACE_MSG(args)    \
          do {                   \
            if (dbug_file())     \
               dbug_printf args; \
          } while (0)

  /* Trace to console
   */
  #define TCP_CONSOLE_MSG(lvl, args) \
          do {                       \
            if (debug_on >= lvl) {   \
              (*_printf) args;       \
              fflush (stdout);       \
            }                        \
          } while (0)
#else
  #define DEBUG_RX(sock, ip)         ((void)0)
  #define DEBUG_TX(sock, ip)         ((void)0)
  #define TCP_TRACE_MSG(args)        ((void)0)
  #define TCP_CONSOLE_MSG(args, lvl) ((void)0)
#endif

#endif

