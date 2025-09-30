#ifndef _w32_TARGET_H
#define _w32_TARGET_H

/*!\file target.h
 *
 * Definitions of targets and macros for Waterloo tcp/ip.
 *
 * by G. Vanem <giva@bgnett.no> 1995 - 2003
 */

#ifndef _w32_WATTCP_H
#error TARGET.H must be included inside or after WATTCP.H
#endif

#define DOS4GW       1             /*!< Tenberry's DOS extender        (1+2)  */
#define DJGPP        2             /*!< GNU C/C++ and djgpp 2.0 target        */
#define PHARLAP      4             /*!< PharLap 386|DosX extender target (1)  */
#define PHAR286      8             /*!< PharLap 286|DosX extender target      */
#define POWERPAK     16            /*!< Borland's PowerPak DOS extender       */
#define X32VM        32            /*!< FlashTek X-32/X-32VM extender         */
#define WINWATT      64            /*!< Experimental; Win32 DLL using WinPcap */
#define PHARLAP_DLL (0x80|PHARLAP) /*!< PharLap DLL version target (not yet)  */
#define DOS4GW_DLL  (0x80|DOS4GW)  /*!< DOS4GW DLL version target (possible?) */
#define DJGPP_DXE   (0x80|DJGPP)   /*!< djgpp DXE target (not yet)            */

/*
 * Notes:
 *
 * (1) Most 32-bit DOS compilers (Borland/Watcom/Microsoft/HighC/DMC)
 *     will work with this DOS extender. Some compilers support far
 *     pointers (48-bits), some don't. And even worse, some of those who
 *     do, have bugs in their segment register handling!
 *     Add `-DBUGGY_FARPTR=1' to your makefile's CFLAGS if you experience
 *     this or crashes in weird places (generate .asm listing to find out).
 *
 *     The problem is present in:
 *       - Metaware's HighC v3.1 at -O3 or above (for sure).
 *       - BCC32 v4, Some rumours say far-ptrs in combination with FPU-ops.
 *
 * (2) Several DOS-extenders supports Watcom-386. DOS4GW (from Tenberry)
 *     is a DPMI 0.9 host with limited API. Other compatible DOS-extenders
 *     can also be used without modifying Watt-32. These are:
 *     DOS4GW Pro, DOS4G, Pmode/W, CauseWay, DOS32A, EDOS, WDOSX and
 *     Zurenava. Watcom with FlashTek's X-32 hasn't been tested.
 *
 */

#ifndef BUGGY_FARPTR
#define BUGGY_FARPTR 0      /* Assume no compilers have fp-bugs, duh! */
#endif

#if defined(_MSC_VER) && defined(M_I86SM)   /* Microsoft doesn't have */
  #define __SMALL__                         /* __SMALL__,  __LARGE__  */
#endif

#if defined(_MSC_VER) && defined(M_I86LM)
  #define __LARGE__
#endif

#if defined(__TINY__) || defined(__COMPACT__) || defined(__MEDIUM__) || defined(__HUGE__)
  #error Unsupported memory model (tiny/compact/medium/huge)
#endif

#if defined(M_I86TM) || defined(M_I86CM) || defined(M_I86MM) || defined(M_I86HM)
  #error Unsupported memory model (tiny/compact/medium/huge)
#endif

#if defined(_M_I86MM) || defined(_M_I86MH)
  #error Unsupported memory model (medium/huge)
#endif

#if defined(__SMALL__) || defined(__LARGE__)
  #undef  DOSX
  #define DOSX 0
#endif

/*
 * djgpp 2.x with GNU C 2.7 or later.
 */
#if defined(__DJGPP__) && defined(__GNUC__)
  #undef  DOSX
  #define DOSX      DJGPP
#endif

/*
 * Watcom 11.x or OpenWatcom 1.x.
 */
#if defined(__WATCOMC__) && defined(__386__)
  #undef  DOSX
  #define DOSX      DOS4GW    /* may be DOS4GW/X32VM/PHARLAP */
  #define WATCOM386
#endif

/*
 * Digital Mars Compiler 8.30+
 */
#if defined(__DMC__) && (__INTSIZE==4)
  #undef  DOSX
  #define DOSX      X32VM     /* may also be DOS4GW/PHARLAP */
  #define DMC386
#endif

/*
 * PellesC works with WIN32 only.
 * Note: "pocc -Ze" sets _MSC_VER.
 */
#if defined(__POCC__)
  #if !defined(__POCC__OLDNAMES)
  #error Use the "pocc -Go" option.
  #endif
  #undef  DOSX
  #define DOSX      WINWATT
  #undef _MSC_VER
#endif

/*
 * Microsoft Visual-C 32-bit. Really not supported with PharLap,
 * but works with WIN32 as target.
 */
#if defined(_MSC_VER) && (_M_IX86 >= 300)
  #undef  DOSX
  #define DOSX      WINWATT  /* VC 2.0 can use PHARLAP too */
  #define MSC386
#endif

/*
 * Metaware's High-C 3.x
 */
#if defined(__HIGHC__)
  #undef  DOSX
  #define DOSX      PHARLAP   /* Is DOS4GW possible? */
#endif

/*
 * Borland 32-bit PowerPak compiler (v4.x, v5.x untested)
 */
#if defined(__BORLANDC__) && defined(__FLAT__) && defined(__DPMI32__)
  #define BORLAND386
  #undef  DOSX
  #define DOSX      POWERPAK  /* may also be DOS4GW (for WDOSX targets) */
                              /* or PHARLAP (not yet) */
#endif

#if defined(__BORLANDC__) && defined(WIN32)
  #define BORLAND_WIN32
  #undef  DOSX
  #define DOSX      WINWATT  /* experimental ATM */
#endif

/*
 * LadSoft (cc386 is rather buggy, so not really supported)
 */
#if defined(__CCDL__) && defined(__386__)
  #undef  DOSX
  #define DOSX      DOS4GW
#endif

/*
 * Build a Windows DLL. Uses WinPcap as packet-driver replacement.
 */
#if defined(WIN32) || defined(_WIN32)
  #undef  DOSX
  #define DOSX      WINWATT
#endif


#ifndef DOSX
  #error DOSX target not defined
#endif

/* 32-bit Digital Mars Compiler defines __SMALL__. So make a
 * define for 16-bit __SMALL__ models.
 */
#if (DOSX == 0) && defined(__SMALL__)
  #define __SMALL16__
#endif

/*
 * Macros and hacks depending on target (DOS-extender).
 */

#if (DOSX & PHARLAP)
  #include <stdio.h>
  #include <pharlap.h>
  #undef FP_OFF
  #include <dos.h>

  #ifdef WATCOM386
  #include <i86.h>
  #endif

  extern REALPTR _watt_dosTbr;
  extern DWORD   _watt_dosTbSize;

  #if (!BUGGY_FARPTR) &&         /* Trust the compiler to handle far-ptr ? */ \
      (__CMPLR_FEATURES__ & __FEATURE_FARPTR__) /* compilers with far-ptrs */
    #define HAS_FP                              /* i.e. HighC, Watcom386   */
    extern FARPTR _watt_dosFp;

    #define DOSMEM(s,o,t) *(t _far*)(_watt_dosFp + (DWORD)((o)|(s)<<4))
    #define PEEKB(s,o)    DOSMEM(s,o,BYTE)
    #define PEEKW(s,o)    DOSMEM(s,o,WORD)
    #define PEEKL(s,o)    DOSMEM(s,o,DWORD)
    #define POKEB(s,o,x)  DOSMEM(s,o,BYTE)  = (BYTE)(x)
    #define POKEW(s,o,x)  DOSMEM(s,o,WORD)  = (WORD)(x)
    #define POKEL(s,o,x)  DOSMEM(s,o,DWORD) = (DWORD)(x)
  #else
    #define PEEKB(s,o)    PeekRealByte (((s) << 16) + (o))
    #define PEEKW(s,o)    PeekRealWord (((s) << 16) + (o))
    #define PEEKL(s,o)    PeekRealDWord(((s) << 16) + (o))
    #define POKEB(s,o,x)  PokeRealByte (((s) << 16) + (o), (x))
    #define POKEW(s,o,x)  PokeRealWord (((s) << 16) + (o), (x))
    #define POKEL(s,o,x)  PokeRealDWord(((s) << 16) + (o), (x))
  #endif

#elif (DOSX & PHAR286)   /* 16-bit protected mode */
  #include <stdio.h>
  #include <phapi.h>

  #error Pharlap Lite not supported yet

#elif (DOSX & DJGPP)
  #include <dos.h>
  #include <dpmi.h>
  #include <go32.h>
  #include <sys/farptr.h>

  #define PEEKB(s,o)      _farpeekb (_dos_ds, (o)+((s)<<4))    /*!< peek at a BYTE in DOS memory */
  #define PEEKW(s,o)      _farpeekw (_dos_ds, (o)+((s)<<4))    /*!< peek at a WORD in DOS memory */
  #define PEEKL(s,o)      _farpeekl (_dos_ds, (o)+((s)<<4))    /*!< peek at a DWORD in DOS memory */
  #define POKEB(s,o,x)    _farpokeb (_dos_ds, (o)+((s)<<4), x) /*!< poke a BYTE to DOS memory */
  #define POKEW(s,o,x)    _farpokew (_dos_ds, (o)+((s)<<4), x) /*!< poke a WORD to DOS memory */
  #define POKEL(s,o,x)    _farpokel (_dos_ds, (o)+((s)<<4), x) /*!< poke a DWORD to DOS memory */
  #define BOOL            int

#elif (DOSX & DOS4GW)
  #if defined(__DJGPP__)
    #include <dpmi.h>
    #include <go32.h>

  #elif defined(__CCDL__)
    #include <dpmi.h>
    #include <i86.h>
  #endif

  #include <dos.h>

  extern WORD  _watt_dosTbSeg;
  extern DWORD _watt_dosTbSize;

  #define DOSMEM(s,o,t)   *(volatile t *) (((s) << 4) | (o))
  #define PEEKB(s,o)      DOSMEM(s,o,BYTE)
  #define PEEKW(s,o)      DOSMEM(s,o,WORD)
  #define PEEKL(s,o)      DOSMEM(s,o,DWORD)
  #define POKEB(s,o,x)    DOSMEM(s,o,BYTE)  = (BYTE)(x)
  #define POKEW(s,o,x)    DOSMEM(s,o,WORD)  = (WORD)(x)
  #define POKEL(s,o,x)    DOSMEM(s,o,DWORD) = (DWORD)(x)
  #undef  BOOL
  #define BOOL int

#elif (DOSX & X32VM)
  #include <x32.h>

  #if defined(DMC386) || defined(WATCOM386)
    #define HAS_FP
    typedef BYTE _far *FARPTR;
    extern FARPTR _watt_dosFp;
  #endif

  extern DWORD _watt_dosTbr, _watt_dosTbSize;

  #ifdef DMC386
    #define DOSMEM(s,o,t) (t*)((DWORD)_x386_zero_base_ptr + (DWORD)((o)|(s)<<4))
    #define PEEKB(s,o)    *DOSMEM(s,o,BYTE)
    #define PEEKW(s,o)    *DOSMEM(s,o,WORD)
    #define PEEKL(s,o)    *DOSMEM(s,o,DWORD)
    #define POKEB(s,o,x)  *DOSMEM(s,o,BYTE)  = (BYTE)(x)
    #define POKEW(s,o,x)  *DOSMEM(s,o,WORD)  = (WORD)(x)
    #define POKEL(s,o,x)  *DOSMEM(s,o,DWORD) = (DWORD)(x)
  #else
    #define PEEKB(s,o)    PeekRealByte (((s) << 16) + (o))
    #define PEEKW(s,o)    PeekRealWord (((s) << 16) + (o))
    #define PEEKL(s,o)    PeekRealDWord(((s) << 16) + (o))
    #define POKEB(s,o,x)  PokeRealByte (((s) << 16) + (o), x)
    #define POKEW(s,o,x)  PokeRealWord (((s) << 16) + (o), x)
    #define POKEL(s,o,x)  PokeRealDWord(((s) << 16) + (o), x)
  #endif

  #undef  BOOL
  #define BOOL int

#elif (DOSX & POWERPAK)
  #include <dos.h>

  extern WORD  _watt_dosTbSeg, _watt_dos_ds;
  extern DWORD _watt_dosTbr, _watt_dosTbSize;

  #define PEEKB(s,o)      _farpeekb (_watt_dos_ds, (o)+((s)<<4))
  #define PEEKW(s,o)      _farpeekw (_watt_dos_ds, (o)+((s)<<4))
  #define PEEKL(s,o)      _farpeekl (_watt_dos_ds, (o)+((s)<<4))
  #define POKEB(s,o,x)    _farpokeb (_watt_dos_ds, (o)+((s)<<4), x)
  #define POKEW(s,o,x)    _farpokew (_watt_dos_ds, (o)+((s)<<4), x)
  #define POKEL(s,o,x)    _farpokel (_watt_dos_ds, (o)+((s)<<4), x)
  #undef  BOOL
  #define BOOL int

#elif !(DOSX & WINWATT)   /* All real-mode and non-WIN32 targets */
  #include <dos.h>
  #define  BOOL           int

  #if defined(__WATCOMC__) || defined(_MSC_VER)
    #define PEEKB(s,o)    (*((BYTE  __far*) MK_FP(s,o)))
    #define PEEKW(s,o)    (*((WORD  __far*) MK_FP(s,o)))
    #define PEEKL(s,o)    (*((DWORD __far*) MK_FP(s,o)))
    #define POKEB(s,o,x)  (*((BYTE  __far*) MK_FP(s,o)) = (BYTE)(x))
    #define POKEW(s,o,x)  (*((WORD  __far*) MK_FP(s,o)) = (WORD)(x))
    #define POKEL(s,o,x)  (*((DWORD __far*) MK_FP(s,o)) = (DWORD)(x))
  #else
    #define PEEKB(s,o)    peekb(s,o)
    #define PEEKW(s,o)    (WORD) peek(s,o)
    #define PEEKL(s,o)    (*((DWORD far*) MK_FP(s,o)))
    #define POKEB(s,o,x)  pokeb (s,o,x)
    #define POKEW(s,o,x)  poke (s,o,x)
    #define POKEL(s,o,x)  (*((DWORD far*) MK_FP(s,o)) = (DWORD)(x))
  #endif
#endif


/* Use Pharlap's definition of a DOS-memory address;
 *   segment in upper 16-bits, offset in lower 16-bits.
 */
#if !(DOSX & PHARLAP) && !defined(REALPTR) && !defined(WIN32)
  #define REALPTR  DWORD
#endif

#if (DOSX)
  #define FARCODE
  #define FARDATA
#else
  #define FARCODE  far
  #define FARDATA  far
#endif

/*
 * Macros and hacks depending on vendor (compiler).
 */

#if defined(__TURBOC__) && (__TURBOC__ <= 0x301)
  #include <mem.h>
  #include <alloc.h>

  #define OLD_TURBOC  /* TCC <= 2.01 doesn't have <malloc.h> and <memory.h> */
  #define __cdecl _Cdecl
  #define _far    far
#elif defined(__POCC__)
  #include <string.h>
  #include <stdlib.h>
#else
  #include <stdlib.h>
  #include <memory.h>
  #include <malloc.h>
#endif

#if defined(__HIGHC__)
  #include <string.h>

  #define max(a,b)        _max (a,b)          /* intrinsic functions */
  #define min(a,b)        _min (a,b)
  #define ENABLE()        _inline (0xFB)      /* sti */
  #define DISABLE()       _inline (0xFA)      /* cli */
#endif

#if defined(_MSC_VER) && !defined(__POCC__)   /* "cl" and not "pocc -Ze" */
  #if (DOSX) && (DOSX != WINWATT) && (DOSX != PHARLAP)
    #error MSC and non-Win32 or non-Pharlap targets not supported
  #endif

  #undef cdecl

  #if (_MSC_VER <= 600)   /* A few exceptions for Quick-C <= 6.0 */
    #define NO_INLINE_ASM /* doesn't have built-in assembler */
    #define ENABLE()      _enable()
    #define DISABLE()     _disable()
    #define cdecl         _cdecl
  #else
 /* #pragma warning (disable:4103 4113 4229) */
 /* #pragma warning (disable:4024 4047 4761 4791) */
    #define ENABLE()      __asm sti
    #define DISABLE()     __asm cli
    #define asm           __asm
    #define cdecl         __cdecl
  #endif

  #if (_MSC_VER >= 800)
    #pragma intrinsic (abs, memcmp, memcpy, memset)
    #pragma intrinsic (strcat, strcmp, strcpy, strlen)
    #if (DOSX == 0)
      #pragma intrinsic (_fmemcpy)
    #endif
  #endif

  #if (DOSX == 0)
    #define interrupt     _interrupt far
    #define getvect(a)    _dos_getvect (a)
    #define setvect(a,b)  _dos_setvect (a, b)
    #undef  MK_FP
    #define MK_FP(s,o)    (void _far*) (((DWORD)(s) << 16) + (DWORD)(o))
  #endif
#endif

#if defined(__TURBOC__) || defined(__BORLANDC__)
  #if defined(__DPMI16__)
    #error 16-bit DPMI version not supported
  #endif

  #include <string.h>

  #define ENABLE()        __emit__(0xFB)
  #define DISABLE()       __emit__(0xFA)

  #pragma warn -bbf-     /* "Bitfields must be signed or unsigned int" warning */
  #pragma warn -sig-     /* "Conversion may loose significant digits" warning */
  #pragma warn -cln-     /* "Constant is long" warning */

  #if defined(__DPMI32__)
    #pragma warn -stu-   /* structure undefined */
    #pragma warn -aus-   /* assigned a value that is never used */
  #endif

  /* make string/memory functions inline */
  #if defined(__BORLANDC__) && !defined(__DPMI32__)
    #define strlen        __strlen__
    #define strncpy       __strncpy__
    #define strrchr       __strrchr__
    #define strncmp       __strncmp__
    #define memset        __memset__
    #define memcpy        __memcpy__
    #define memcmp        __memcmp__
    #define memchr        __memchr__
  #endif
#endif

#if defined(__GNUC__)
  #if (__GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7))
    #error I need GCC 2.7 or later
  #endif
  #include <string.h>

  #define __fastcall      __attribute__((__fastcall__)) /* unsupported in gcc/djgpp */
  #define _fastcall       __attribute__((__fastcall__))

  #if !(DOSX & WINWATT)
    #define ENABLE()      __asm__ __volatile__ ("sti")
    #define DISABLE()     __asm__ __volatile__ ("cli")
  #endif
#endif

#if defined(__WATCOMC__)
  #include <dos.h>
  #include <string.h>

  #if !defined (_M_IX86)
  #error Only Watcom x86 supported
  #endif

  #pragma intrinsic (strcmp, memset)
  #pragma warning (disable:120)
  #if (__WATCOMC__ >= 1220)  /* OW 1.2+ */
  /* #pragma warning (disable:H3006 H3007) */
  #endif

  #if !(DOSX & WINWATT)
    #define getvect(a)      _dos_getvect (a)
    #define setvect(a,b)    _dos_setvect (a, b)
    #define BOOL            int

    extern void ENABLE  (void);
    extern void DISABLE (void);
    #pragma aux ENABLE  = 0xFB;
    #pragma aux DISABLE = 0xFA;
  #endif
#endif

#if defined(__DMC__) && !(DOSX & WINWATT)
  #include <dos.h>
  #include <dpmi.h>
  #include <string.h>

  #define ENABLE()        enable()
  #define DISABLE()       disable()
#endif

#if defined(__CCDL__) && !(DOSX & WINWATT)
  #include <string.h>
  #include <dos.h>

  #define cdecl           _cdecl
  #define BOOL            int
  #define ENABLE()        asm sti
  #define DISABLE()       asm cli
#endif

#if (DOSX & WINWATT)
  extern CRITICAL_SECTION _watt_crit_sect;

  #if defined(__LCC__)
    #define ENTER_CRIT()    EnterCriticalSection ((struct _CRITICAL_SECTION*)&_watt_crit_sect)
    #define LEAVE_CRIT()    LeaveCriticalSection ((struct _CRITICAL_SECTION*)&_watt_crit_sect)
  #else
    #define ENTER_CRIT()    EnterCriticalSection (&_watt_crit_sect)
    #define LEAVE_CRIT()    LeaveCriticalSection (&_watt_crit_sect)
  #endif

  #undef  DISABLE
  #define DISABLE()       ENTER_CRIT()
  #undef  ENABLE
  #define ENABLE()        LEAVE_CRIT()
#else
  #define ENTER_CRIT()    ((void)0)
  #define LEAVE_CRIT()    ((void)0)
#endif


#if !defined(cdecl) && defined(__GNUC__) && defined(__MINGW32__)
#define cdecl __attribute__((__cdecl__))
#endif

#ifndef cdecl
#define cdecl
#endif

#if (DOSX & (DOS4GW|X32VM))
#define HAVE_DOS_NEAR_PTR
#endif

#ifndef max
#define max(a,b)   (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)   (((a) < (b)) ? (a) : (b))
#endif

/*
 * Macros to hide GNU-C features.
 */
#if defined(__GNUC__) && (__GNUC__ >= 3) && \
   !defined(WATT32_DOS_DLL)  /* not djgpp for DXE */
  #define ATTR_PRINTF(_1,_2) __attribute__((format(printf,_1,_2)))
  #define ATTR_SCANF(_1,_2)  __attribute__((format(scanf,_1,_2)))
#else
  #define ATTR_PRINTF(_1,_2)
  #define ATTR_SCANF(_1,_2)
#endif

#if defined(__GNUC__)
  #define ATTR_REGPARM(_n)   __attribute__((regparm(_n)))
  #define ATTR_NORETURN()    __attribute__((noreturn))
#else
  #define ATTR_REGPARM(n)
  #define ATTR_NORETURN()
#endif

#if defined(__LCC__) || defined(__POCC__)
  #define __FUNCTION__  __func__
#elif defined(_MSC_VER) && (_MSC_VER >= 1300)
  /* __FUNCTION__ is built-in */
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
  #define __FUNCTION__  __func__
#elif !defined(__GNUC__) && !defined(__WATCOMC__) && !defined(__DMC__)
  #define __FUNCTION__  "func?"
#endif


/*
 * Hack for Microsoft C.
 *
 * Quick-C/Visual-C insists that signal-handlers, atexit() and var-arg
 * functions must be defined cdecl. But e.g. Watcom's register call
 * doesn't need this. This becomes a problem if using _fastcall or
 * "cl /Gr" globally.
 */
#if defined(_MSC_VER) && !defined(__POCC__) && !defined(WIN32)
  #undef  MS_CDECL
  #define MS_CDECL  cdecl
#else
  #define MS_CDECL
#endif


/*
 * Because kbhit() will pull in more conio function that we
 * really need, use the simple kbhit() variant (without ungetch
 * option). This also prevents multiple definition trouble when
 * linking e.g. PD-curses and Watt-32 library.
 */

#if defined (__DJGPP__)
  #ifdef __dj_include_conio_h_
    #error "Don't include <conio.h>"
  #endif
  #include <pc.h>     /* simple kbhit() */

#elif defined (__HIGHC__) || defined(__WATCOMC__) || defined(__DMC__)
  #if defined(__portable_conio_h_) || defined(__metaware_conio_h_)
    #error "Don't include <mw/conio.h>"
  #endif
  #include <conio.h>  /* simple kbhit() */

#elif defined(_MSC_VER) && defined(__386__)
  /*
   * Problems including <conio.h> from Visual C 4.0
   */
  int __cdecl kbhit (void);

#else                 /* no other option */
  #include <conio.h>
#endif

#endif  /* _w32_TARGET_H */

