/*!\file misc.h
 */
#ifndef _w32_MISC_H
#define _w32_MISC_H

#ifndef __SYS_SWAP_BYTES_H
#include <sys/swap.h>       /* intel() etc. */
#endif

#ifndef __SYS_WTIME_H
#include <sys/wtime.h>      /* struct timeval */
#endif

#if defined(__DJGPP__) || defined(__HIGHC__) || defined(__WATCOMC__) || \
    defined(__DMC__)
  #undef _WIN32       /* Needed for __DMC__ */
  #include <unistd.h>
#endif

#if defined(__LCC__) && !defined(W32_LCC_INTRINSICS_INCLUDED)
  #include <intrinsics.h>
#elif defined(__POCC__) && 0
  #include <intrin.h>
#endif

/*
 * In misc.c
 *
 * Protect internal symbols with a namespace
 */
#define init_misc       NAMESPACE (init_misc)
#define Random          NAMESPACE (Random)
#define RandomWait      NAMESPACE (RandomWait)
#define Wait            NAMESPACE (Wait)
#define valid_addr      NAMESPACE (valid_addr)
#define is_in_stack     NAMESPACE (is_in_stack)
#define used_stack      NAMESPACE (used_stack)
#define os_yield        NAMESPACE (os_yield)
#define hex_chars_lower NAMESPACE (hex_chars_lower)
#define hex_chars_upper NAMESPACE (hex_chars_upper)
#define win32_dos_box   NAMESPACE (win32_dos_box)
#define get_day_num     NAMESPACE (get_day_num)
#define dword_str       NAMESPACE (dword_str)
#define shell_exec      NAMESPACE (shell_exec)
#define get_mem_strat   NAMESPACE (get_mem_strat)
#define set_mem_strat   NAMESPACE (set_mem_strat)
#define rundown_add     NAMESPACE (rundown_add)
#define rundown_run     NAMESPACE (rundown_run)
#define rundown_dump    NAMESPACE (rundown_dump)
#define assert_fail     NAMESPACE (assert_fail)
#define memdbg_init     NAMESPACE (memdbg_init)
#define fopen_excl      NAMESPACE (fopen_excl)

#if !defined(SEARCH_LIST_DEFINED)
  #define SEARCH_LIST_DEFINED
  struct search_list {
         DWORD       type;
         const char *name;
       };
#endif

#define DO_FREE(x)  do {           \
                      if (x) {     \
                         free (x); \
                         x = NULL; \
                      }            \
                    } while (0)

#if defined(WIN32)
  #define SOCK_ERRNO(err)  WSASetLastError (err)
#else
  #define SOCK_ERRNO(err)  errno = _w32_errno = err
#endif


#if !defined(__DJGPP__)
  W32_DATA int __bss_count;
#endif

W32_DATA BOOL     win32_dos_box;
W32_DATA BOOL    _watt_fatal_error;
W32_DATA WORD    _watt_os_ver;
W32_DATA char    _watt_assert_buf[256];

extern const char hex_chars_lower[];
extern const char hex_chars_upper[];

W32_FUNC void     init_misc    (void);
W32_FUNC unsigned Random       (unsigned a, unsigned b);
W32_FUNC void     RandomWait   (unsigned msec_a, unsigned msec_b);
W32_FUNC void     Wait         (unsigned msec);
W32_FUNC BOOL     valid_addr   (DWORD addr, unsigned len);
W32_FUNC BOOL     is_in_stack  (const void *ptr);
W32_FUNC unsigned used_stack   (void);

W32_FUNC void     os_yield     (void);
W32_FUNC BOOL     watt_kbhit   (void);

extern   DWORD    get_day_num  (void);
extern   BOOL     shell_exec   (const char *cmd);
extern   BOOL     get_mem_strat(BYTE *strat);
extern   BOOL     set_mem_strat(BYTE strat);
extern   void     memdbg_init  (void);
extern   int      rundown_add  (void (*func)(void), const char *name,
                                int order, const char *file, unsigned line);
extern   void     rundown_run  (void);
extern   void     rundown_dump (void);
extern   FILE    *fopen_excl   (const char *file, const char *mode);

extern const char *dos_extender_name (void);
extern void assert_fail (const char *file, unsigned line, const char *what);

extern const char *dword_str (DWORD val);

#define RUNDOWN_ADD(func,order) \
        rundown_add ((void(*)(void))func, #func, order, __FILE__, __LINE__)

/*
 * Yielding while waiting on network reduces CPU-loading.
 */
#if (DOSX & (DJGPP|DOS4GW|PHARLAP|X32VM|WINWATT))
  #define WATT_YIELD()   os_yield()  /* includes kbhit() for non-djgpp */
#else
  #define WATT_YIELD()   watt_kbhit()
#endif

#if defined(USE_DEBUG)
  #define list_lookup    NAMESPACE (list_lookup)
  #define MAC_address    NAMESPACE (MAC_address)
  #define unfinished     NAMESPACE (unfinished)

  extern const char *list_lookup (DWORD, const struct search_list *, int);
  extern const char *MAC_address (const void *addr);
  extern void        unfinished (const char *func, const char *file, unsigned line);

  #define UNFINISHED()  unfinished (__FUNCTION__, __FILE__, __LINE__)
#else
  #define UNFINISHED()  ((void)0)
#endif

#if defined(__LARGE__)
  extern void watt_large_check (const void *sock, int size,
                                const char *file, unsigned line);
  #define WATT_LARGE_CHECK(sock, size) \
          watt_large_check(sock, size, __FILE__, __LINE__)
#else
  #define WATT_LARGE_CHECK(sock, size)  ((void)0)
#endif

#if defined(USE_DEBUG) && !defined(NDEBUG)
  #define WATT_ASSERT(x)  do { \
                            if (!(x)) \
                               assert_fail (__FILE__, __LINE__, #x); \
                          } while (0)
#else
  #define WATT_ASSERT(x)  ((void)0)
#endif


#if defined(WIN32)
  /*
   * In winmisc.c
   */
  #define init_winmisc       NAMESPACE (init_winmisc)
  #define win_strerror       NAMESPACE (win_strerror)
  #define assert_fail_win    NAMESPACE (assert_fail_win)

  #define WinDnsQueryA4      NAMESPACE (WinDnsQueryA4)
  #define WinDnsQueryA6      NAMESPACE (WinDnsQueryA6)
  #define WinDnsQueryPTR4    NAMESPACE (WinDnsQueryPTR4)
  #define WinDnsQueryPTR6    NAMESPACE (WinDnsQueryPTR6)
  #define WinDnsCachePut_A4  NAMESPACE (WinDnsCachePut_A4)
  #define WinDnsCachePut_A6  NAMESPACE (WinDnsCachePut_A6)

  extern BOOL  _watt_is_win9x;

  extern BOOL  init_winmisc    (void);
  extern char *win_strerror    (DWORD err);
  extern void  assert_fail_win (const char *file, unsigned line,
                                const char *what);

  extern BOOL  WinDnsQuery_A4    (const char *name, DWORD *ip4);
  extern BOOL  WinDnsQuery_A6    (const char *name, void *ip6);
  extern BOOL  WinDnsQuery_PTR4  (DWORD ip4, char *name, size_t size);
  extern BOOL  WinDnsQuery_PTR6  (const void *ip6, char *name, size_t size);
  extern BOOL  WinDnsCachePut_A4 (const char *name, DWORD ip4);
  extern BOOL  WinDnsCachePut_A6 (const char *name, const void *ip6);

  #if defined(USE_DEBUG) && !defined(NDEBUG)
    #define WIN_ASSERT(x)   do {        \
                              if (!(x)) \
                                 assert_fail_win (__FILE__, __LINE__, #x); \
                            } while (0)
  #else
    #define WIN_ASSERT(x)  ((void)0)
  #endif
#endif


#if defined(HAVE_UINT64)
  /*
   * Format for printing 64-bit types. E.g. "%10"S64_FMT
   */
  #if defined(_MSC_VER) || defined(_MSC_EXTENSIONS) || defined(__WATCOMC__) || \
      defined(__LCC__) || defined(__BORLANDC__)
    #define S64_FMT        "I64d"
    #define U64_FMT        "I64u"
    #define S64_SUFFIX(x)  (x##i64)
    #define U64_SUFFIX(x)  (x##Ui64)
  #else
    #define S64_FMT        "Ld"
    #define U64_FMT        "Lu"
    #define S64_SUFFIX(x)  (x##LL)
    #define U64_SUFFIX(x)  (x##ULL)
  #endif
#endif



/*
 * In pc_cbrk.c
 */
W32_DATA WORD         _watt_handle_cbreak;
W32_DATA volatile int _watt_cbroke;

W32_FUNC int           tcp_cbreak (int mode);
extern   int           set_cbreak (int want_brk);
extern   void MS_CDECL sig_handler_watt (int sig);

#define NEW_BREAK_MODE(old,new) \
        old = _watt_handle_cbreak, \
        _watt_handle_cbreak = new,  /* enable special interrupt mode */ \
        _watt_cbroke = 0

#define OLD_BREAK_MODE(old) \
        _watt_cbroke = 0,   /* always clean up */ \
        _watt_handle_cbreak = old


/*
 * In country.c (no longer used. Maybe use with IDNA code?)
 */
#define GetCountryCode  NAMESPACE (GetCountryCode)
#define GetCodePage     NAMESPACE (GetCodePage)

extern int GetCountryCode (void);
extern int GetCodePage (void);


#if !defined(WIN32)
  /*
   * In crit.c; Sets up a critical error handler.
   * No longer used.
   */
  #define CRIT_VECT      0x24
  #define int24_init     NAMESPACE (int24_init)
  #define int24_restore  NAMESPACE (int24_restore)

  extern void int24_init (void);
  extern void int24_restore (void);

  /*
   * In qmsg.c
   */
  #define dbg_colour    NAMESPACE (dbg_colour)
  #define dbg_scrpos    NAMESPACE (dbg_scrpos)
  #define dbg_scrstart  NAMESPACE (dbg_scrstart)
  #define dputch        NAMESPACE (dputch)
  #define dmsg          NAMESPACE (dmsg)
  #define dhex1int      NAMESPACE (dhex1int)
  #define dhex2int      NAMESPACE (dhex2int)
  #define dhex4int      NAMESPACE (dhex4int)
  #define dhex8int      NAMESPACE (dhex8int)

  extern BYTE dbg_colour;
  extern WORD dbg_scrpos;
  extern WORD dbg_scrstart;

  extern void dputch   (char x);
  extern void dmsg     (const char *s);
  extern void dhex1int (int x);
  extern void dhex2int (int x);
  extern void dhex4int (int x);
  extern void dhex8int (DWORD x);

#endif


/*
 * In version.c
 */
W32_DATA const char *wattcpCopyright;
W32_FUNC const char *wattcpVersion (void);
W32_FUNC const char *wattcpCapabilities (void);


/*
 * In crc.c
 */
#define crc_init   NAMESPACE (crc_init)
#define crc_bytes  NAMESPACE (crc_bytes)

extern BOOL  crc_init  (void);
extern DWORD crc_bytes (const char *buf, unsigned len);

/*
 * In neterr.c
 */
#define short_strerror      NAMESPACE (short_strerror)
#define pull_neterr_module  NAMESPACE (pull_neterr_module)

extern int         pull_neterr_module;
extern const char *short_strerror (int errnum);


/*
 * IREGS structures for pkt_api_entry() etc.
 * Defines to simplify issuing real-mode interrupts
 */
#if (DOSX & PHARLAP)
  #define IREGS      SWI_REGS     /* in Pharlap's <pharlap.h> */
  #define r_flags    flags
  #define r_ax       eax
  #define r_bx       ebx
  #define r_dx       edx
  #define r_cx       ecx
  #define r_si       esi
  #define r_di       edi
  #define r_ds       ds
  #define r_es       es
  #define r_fs       fs
  #define r_gs       gs

#elif (DOSX & DJGPP)
  #define IREGS      __dpmi_regs  /* in <dpmi.h> */
  #define r_flags    x.flags
  #define r_ax       d.eax
  #define r_bx       d.ebx
  #define r_dx       d.edx
  #define r_cx       d.ecx
  #define r_si       d.esi
  #define r_di       d.edi
  #define r_ip       x.ip
  #define r_cs       x.cs
  #define r_ds       x.ds
  #define r_es       x.es
  #define r_fs       x.fs
  #define r_gs       x.gs
  #define r_ss       x.ss
  #define r_sp       x.sp

#elif (DOSX & X32VM)
  #define IREGS      SWI_REGS           /* in "x32vm.h" */

#elif defined(__CCDL__)
  #define IREGS      union _dpmi_regs_  /* in <dpmi.h> */
  #define r_flags    h.flags
  #define r_ax       d.eax
  #define r_bx       d.ebx
  #define r_dx       d.edx
  #define r_cx       d.ecx
  #define r_si       d.esi
  #define r_di       d.edi
  #define r_ds       h.ds
  #define r_es       h.es
  #define r_fs       h.fs
  #define r_gs       h.gs

#elif (DOSX & (DOS4GW|POWERPAK))
  typedef struct DPMI_regs {
          DWORD  r_di;
          DWORD  r_si;
          DWORD  r_bp;
          DWORD  reserved;
          DWORD  r_bx;
          DWORD  r_dx;
          DWORD  r_cx;
          DWORD  r_ax;
          WORD   r_flags;
          WORD   r_es, r_ds, r_fs, r_gs;
          WORD   r_ip, r_cs, r_sp, r_ss;
        } IREGS;

#elif (DOSX & WINWATT)

#else  /* r-mode targets */

  /* IREGS must have same layout and size as Borland's 'struct REGPACK'
   * and Watcom's 'union REGPACK'. This is checked in `check_reg_struct()'.
   */
  #include <sys/packon.h>

  typedef struct IREGS {
          WORD  r_ax, r_bx, r_cx, r_dx, r_bp;
          WORD  r_si, r_di, r_ds, r_es, r_flags;
        } IREGS;

  #include <sys/packoff.h>

  #if (defined(_MSC_VER) || defined(__DMC__)) && (DOSX == 0)
    #define intr NAMESPACE (intr)
    extern void intr (int int_no, IREGS *regs);
  #endif
#endif

#define CARRY_BIT  1   /* LSB in r_flags */


/* Macro to ease gneration of real-mode interrupts.
 * `_r' should be `IREGS'.
 */
#if (DOSX & DJGPP)
  #define GEN_INTERRUPT(_i,_r)   __dpmi_int ((int)(_i), _r)

#elif (DOSX & (PHARLAP|X32VM))
  #define GEN_INTERRUPT(_i,_r)   _dx_real_int ((UINT)(_i), _r)

#elif defined(__CCDL__)
  #define GEN_INTERRUPT(_i,_r)   dpmi_simulate_real_interrupt (_i, _r)

#elif (DOSX & (DOS4GW|POWERPAK))
  #define GEN_INTERRUPT(_i,_r)   dpmi_real_interrupt ((_i), _r)

#elif (DOSX == 0)
  #if defined(_MSC_VER) || defined(__DMC__)
    #define GEN_INTERRUPT(_i,_r)  intr ((int)(_i), _r) /* our own version */

  #elif defined(__WATCOMC__)
    #define GEN_INTERRUPT(_i,_r)  intr ((int)(_i), (union REGPACK*)(_r))

  #else
    #define GEN_INTERRUPT(_i,_r)  intr ((int)(_i), (struct REGPACK*)(_r))
  #endif

#elif (DOSX & WINWATT)
  /* No interrupts on Win32 */

#else
  #error Help, unknown target.
#endif


/*
 * Fixes for Quick-C and Digital Mars to allow an address as lvalue
 * in FP_SEG/OFF macros (only used in wdpmi.c)
 */
#if (!DOSX)
  #if defined(_MSC_VER)
  /*#pragma warning (disable:4759) */
    #undef  FP_SEG
    #undef  FP_OFF
    #define FP_SEG(p) ((unsigned)(_segment)(void _far *)(p))
    #define FP_OFF(p) ((unsigned)(p))

  #elif defined(__DMC__)
    #undef  FP_SEG
    #undef  FP_OFF
    #define FP_SEG(p) ((unsigned)((DWORD)(void _far*)(p) >> 16))
    #define FP_OFF(p) ((unsigned)(p))
  #endif

#elif defined(BORLAND386)
  #undef  FP_SEG
  #undef  FP_OFF
  #define FP_SEG(p)   _DS  /* segment of something is always our DS */
  #define FP_OFF(p)   (DWORD)(p)

#elif (defined(DMC386) || defined(MSC386)) && !defined(WIN32)
  #undef  FP_SEG
  #undef  FP_OFF
  #define FP_SEG(p)   ((unsigned)((DWORD)(void _far*)(p) >> 16))
  #define FP_OFF(p)   ((unsigned)(p))
#endif


#if (defined(MSC386) || defined(__HIGHC__)) && !defined(WIN32)
  #include <pldos32.h>
  #define dosdate_t  _dosdate_t
  #define dostime_t  _dostime_t
#endif

#if defined(__DMC__) && !defined(WIN32)
  #define dosdate_t  dos_date_t
  #define dostime_t  dos_time_t
#endif


/*
 * The Watcom STACK_SET() function is from Dan Kegel's RARP implementation
 */
#if defined(__WATCOMC__) && !defined(__386__)  /* 16-bit Watcom */
  extern void STACK_SET (void far *stack);
  #pragma aux STACK_SET = \
          "mov  ax, ss"   \
          "mov  bx, sp"   \
          "mov  ss, dx"   \
          "mov  sp, si"   \
          "push ax"       \
          "push bx"       \
          parm [dx si]    \
          modify [ax bx];

  extern void STACK_RESTORE (void);
  #pragma aux STACK_RESTORE = \
          "pop bx"            \
          "pop ax"            \
          "mov ss, ax"        \
          "mov sp, bx"        \
          modify [ax bx];

  extern void PUSHF_CLI (void);
  #pragma aux PUSHF_CLI = \
          "pushf"         \
          "cli";

  extern void POPF (void);
  #pragma aux POPF = \
          "popf";

#elif defined(WATCOM386)       /* 32-bit Watcom targets */
  extern void STACK_SET (void *stack);
  #pragma aux STACK_SET =  \
          "mov  ax, ss"    \
          "mov  ebx, esp"  \
          "mov  cx, ds"    \
          "mov  ss, cx"    \
          "mov  esp, esi"  \
          "push eax"       \
          "push ebx"       \
          parm [esi]       \
          modify [eax ebx ecx];

  extern void STACK_RESTORE (void);
  #pragma aux STACK_RESTORE = \
          "lss esp, [esp]";

  extern void PUSHF_CLI (void);
  #pragma aux PUSHF_CLI = \
          "pushfd"        \
          "cli";

  extern void POPF (void);
  #pragma aux POPF = \
          "popfd";

  extern WORD watcom_MY_CS (void);
  #pragma aux watcom_MY_CS = \
          "mov ax, cs"       \
          modify [ax];

  extern WORD watcom_MY_DS(void);
  #pragma aux watcom_MY_DS = \
          "mov ax, ds"       \
          modify [ax];

  extern DWORD GET_LIMIT (WORD sel);
  #pragma aux GET_LIMIT = \
          ".386p"         \
          "lsl eax, eax"  \
          parm [eax];

  #define get_cs_limit() GET_LIMIT (watcom_MY_CS())
  #define get_ds_limit() GET_LIMIT (watcom_MY_DS())

#elif defined(BORLAND386) || defined(DMC386) || defined(MSC386)
  #define STACK_SET(stk)  __asm { mov  ax, ss;   \
                                  mov  ebx, esp; \
                                  mov  cx, ds;   \
                                  mov  ss, cx;   \
                                  mov  esp, stk; \
                                  push eax;      \
                                  push ebx       \
                                }

  #define STACK_RESTORE() __asm lss esp, [esp]
  #define PUSHF_CLI()     __emit__ (0x9C,0xFA)  /* pushfd; cli */
  #define POPF()          __emit__ (0x9D)       /* popfd */

  extern DWORD get_ds_limit (void);
  extern DWORD get_cs_limit (void);
  extern DWORD get_ss_limit (void);  /* only needed for X32 */

#elif defined(__CCDL__)
  #define STACK_SET(stk)  asm { mov  ax, ss;   \
                                mov  ebx, esp; \
                                mov  cx, ds;   \
                                mov  ss, cx;   \
                                mov  esp, stk; \
                                push eax;      \
                                push ebx       \
                              }

  #define STACK_RESTORE() asm lss esp, [esp]
  #define PUSHF_CLI()     asm { pushfd; cli }
  #define POPF()          asm popfd

  extern DWORD get_ds_limit (void);
  extern DWORD get_cs_limit (void);

#elif defined(__HIGHC__)
  #define PUSHF_CLI()     _inline (0x9C,0xFA)  /* pushfd; cli */
  #define POPF()          _inline (0x9D)       /* popfd */

#elif defined(__GNUC__)
  #define PUSHF_CLI()     __asm__ __volatile__ ("pushfl; cli" ::: "memory")
  #define POPF()          __asm__ __volatile__ ("popfl"       ::: "memory")

#elif defined(__SMALL16__) || defined(__LARGE__)
  #if defined(__BORLANDC__) /* prevent spawning tasm.exe */
    #define PUSHF_CLI()   __emit__ (0x9C,0xFA)
    #define POPF()        __emit__ (0x9D)

  #elif defined(__DMC__) || (defined(_MSC_VER) && !defined(NO_INLINE_ASM))
    #define PUSHF_CLI()   __asm pushf; \
                          __asm cli
    #define POPF()        __asm popf
  #endif
#endif

#if !defined(__WATCOMC__)
  #ifndef PUSHF_CLI
  #define PUSHF_CLI() ((void)0)
  #endif

  #ifndef POPF
  #define POPF()      ((void)0)
  #endif
#endif


#define BEEP_FREQ   7000
#define BEEP_MSEC   1

#if defined(WIN32)
  #define BEEP()    MessageBeep (MB_OK)

#elif defined(_MSC_VER)
  #define BEEP()    ((void)0) /* doesn't even have delay() :-< */

#elif defined(__DMC__)
  #define BEEP()    sound_note (BEEP_FREQ, BEEP_MSEC)

#elif defined(__DJGPP__)
  #define BEEP()    do { \
                      sound (BEEP_FREQ); \
  /* delay() don't */ usleep (1000*BEEP_MSEC); \
  /* work under NT */ nosound(); \
                    } while (0)

#elif defined(__CCDL__)
  #define BEEP()    do { \
                      _sound (BEEP_FREQ); \
                      _delay (BEEP_MSEC); \
                      _nosound(); \
                    } while (0)
#else
  #define BEEP()    do { \
                      sound (BEEP_FREQ); \
                      delay (BEEP_MSEC); \
                      nosound(); \
                    } while (0)
#endif

/*
 * Defines for real/pmode-mode interrupt handlers (crit.c/netback.c/pcintr.c)
 */
#if !defined(INTR_PROTOTYPE)
#if (DOSX == 0)
  #if (__WATCOMC__ >= 1200)   /* OpenWatcom 1.0+ */
    #define INTR_PROTOTYPE  void interrupt far
    typedef INTR_PROTOTYPE (*W32_IntrHandler)();

  #elif defined(__TURBOC__)
    #define INTR_PROTOTYPE  void cdecl interrupt
    typedef void interrupt (*W32_IntrHandler)(void);

  #else
    #define INTR_PROTOTYPE  void cdecl interrupt
    typedef void (cdecl interrupt *W32_IntrHandler)();
  #endif

#elif defined(WIN32)
  /* No need for this on Windows */

#elif defined(WATCOM386)
  #define INTR_PROTOTYPE  void __interrupt __far
  typedef INTR_PROTOTYPE (*W32_IntrHandler)();

#elif defined(__HIGHC__)
  #define INTR_PROTOTYPE  _Far void _CC (_INTERRUPT|_CALLING_CONVENTION)
  typedef INTR_PROTOTYPE (*W32_IntrHandler)();

#elif defined(__DJGPP__)
  #define INTR_PROTOTYPE  void   /* simply a SIGALRM handler */
  typedef void (*W32_IntrHandler)(int);

#else
  /* Figure out what to do for others .. */
#endif
#endif  /* INTR_PROTOTYPE */


#if defined(__GNUC__) && defined(__i386__) /* also for gcc -m486 and better */
 /*
  * This is not used yet since the benefit/drawbacks are unknown.
  * Define 32-bit multiplication asm macros.
  *
  * umul_ppmm (high_prod, low_prod, multiplier, multiplicand)
  * multiplies two unsigned long integers multiplier and multiplicand,
  * and generates a two unsigned word product in high_prod and
  * low_prod.
  */
  #define umul_ppmm(w1,w0,u,v)                           \
          __asm__ __volatile__ (                         \
                 "mull %3"                               \
               : "=a" ((DWORD)(w0)), "=d" ((DWORD)(w1))  \
               : "%0" ((DWORD)(u)),  "rm" ((DWORD)(v)))

  #define mul32(u,v) ({ union ulong_long w;               \
                        umul_ppmm (w.s.hi, w.s.lo, u, v); \
                        w.ull; })
  /* Use as:
   *  DWORD x,y;
   *  ..
   *  uint64 z = mul32 (x,y);
   */

  /* Borrowed from djgpp's <sys/farptr.h>
   * Needed in pkt_receiver_pm() because djgpp 2.03 doesn't
   * save/restore FS/GS registers in the rmode callback stub.
   * Not needed if 'USE_FAST_PKT' is used.
   */
  _W32_EXTERN_INLINE WORD get_fs_reg (void)
  {
     WORD sel;
     __asm__ __volatile__ (
             "movw %%fs, %w0"
           : "=r" (sel) : );
     return (sel);
  }
  _W32_EXTERN_INLINE WORD get_gs_reg (void)
  {
     WORD sel;
     __asm__ __volatile__ (
             "movw %%gs, %w0"
           : "=r" (sel) : );
     return (sel);
  }
  _W32_EXTERN_INLINE void set_fs_reg (WORD sel)
  {
    __asm__ __volatile__ (
            "movw %w0, %%fs"
         :: "rm" (sel));
  }
  _W32_EXTERN_INLINE void set_gs_reg (WORD sel)
  {
    __asm__ __volatile__ (
            "movw %w0, %%gs"
         :: "rm" (sel));
  }
#else
  #define get_fs_reg()    (1)
  #define get_gs_reg()    (1)
  #define set_fs_reg(fs)  ((void)0)
  #define set_gs_reg(gs)  ((void)0)
#endif /* __GNUC__ && __i386__ */


/*
 * Various macros
 *
 * Note: Some versions of snprintf return -1 if they truncate
 *       the output. Others return size of truncated output.
 */
#undef SNPRINTF
#undef VSNPRINTF

#if defined(WIN32)
  #define SNPRINTF   _snprintf
  #define VSNPRINTF  _vsnprintf

#elif defined(__HIGHC__) || defined(__WATCOMC__)
  #define SNPRINTF   _bprintf
  #define VSNPRINTF  _vbprintf

#elif defined(__DMC__) || (defined(_MSC_VER) && (_MSC_VER >= 700))
  #define SNPRINTF   _snprintf
  #define VSNPRINTF  _vsnprintf

#elif defined(__DJGPP__) && (__DJGPP_MINOR__ >= 4)
  #define SNPRINTF   snprintf
  #define VSNPRINTF  vsnprintf
#endif

#define UNCONST(type, var, val)   (*(type *)&(var)) = val

/*
 * Some prototypes not found in SP-lint's libraries
 */
#if defined(lint) || defined(_lint)
  extern char *strupr (char*);
  extern char *strlwr (char*);
  extern char *strdup (const char*);
  extern int   stricmp (const char*, const char*);
  extern int   strnicmp (const char*, const char*, size_t);
  extern char *itoa (int, char*, int);
  extern void *alloca (size_t);
  extern int   gettimeofday (struct timeval*, struct timezone*);
  extern int   fileno (FILE*);
  extern int   isascii (int);
  extern void  psignal (int, const char*);
  extern int   vsscanf (const char*, const char*, va_list);

  /*@constant int SIGALRM; @*/
  /*@constant int SIGTRAP; @*/
  /*@constant int EINVAL;  @*/
  /*@constant int EFAULT;  @*/
#endif

#endif /* _w32_MISC_H */

