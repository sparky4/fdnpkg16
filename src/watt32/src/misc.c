/*!\file misc.c
 *
 *  Module for various things:
 *   - host/network order (little/big endian) swapping of bytes.
 *   - initialise PEEK/POKE macros.
 *   - allocate transfer buffer for DOSX targets.
 *   - address validation for DOSX targets.
 *   - simple range limited random routine.
 *   - ffs routine to find the first bit set.
 *   - initialise memory-debugger (Fortify/CrtDbg)
 *   - stack checker exit routine for Borland/Watcom.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>

#ifdef __HIGHC__
#include <init.h>  /* _mwlsl(), _msgetcs() */
#endif

#if defined(WIN32) || defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>
#endif

#include "wattcp.h"
#include "wdpmi.h"
#include "x32vm.h"
#include "powerpak.h"
#include "strings.h"
#include "sock_ini.h"
#include "cpumodel.h"
#include "pcsed.h"
#include "pcpkt.h"
#include "pcconfig.h"
#include "pcdbug.h"
#include "bsddbug.h"
#include "ioport.h"
#include "btrace.h"
#include "timer.h"
#include "misc.h"

/* These arrays are used in several places (save some space)
 */
const char hex_chars_lower[] = "0123456789abcdef";
const char hex_chars_upper[] = "0123456789ABCDEF";

BOOL win32_dos_box     = FALSE;  /*!< Running under Windows DOS-box */
BOOL _watt_fatal_error = FALSE;  /*!< We're exiting via an exception handler */
WORD _watt_os_ver      = 0x622;  /*!< True DOS/WinNT version MSB.LSB */
char _watt_assert_buf[256];      /*!< Print this in dbug_exit() */

/*
 * Set through macro SOCK_ERRNO() in <sys/werrno.h>.
 */
int _w32_errno = 0;

#if !defined(__DJGPP__)
  int __bss_count = 0;    /** \todo detect program restarts */
#endif

#if (DOSX & (PHARLAP|X32VM)) && defined(HAS_FP) /* has 48-bit far-pointers */
  FARPTR _watt_dosFp;
#endif

#if defined(__LCC__)
  static _CPUID lcc_cpuid;
  char          cdecl x86_type = 5;      /* !! */
  char          cdecl x86_vendor_id[13];
  DWORD         cdecl x86_capability;
#endif

#if (DOSX)
  static void setup_dos_xfer_buf (void);
  static void is_in_stack_init (void);
#endif

#if defined(__BORLANDC__)
  #pragma inline
  #if defined(USE_DEBUG) && (defined(__SMALL__) || defined(__LARGE__))
  STATIC int setup_stk_check (void);
  #endif

#elif defined(WATCOM386) && !defined(WIN32)
  extern char cdecl __begtext;    /* label at TEXT start */
  extern UINT cdecl _x386_stacklow;

  #if defined(__SW_3S)            /* wcc386 -3s */
    void cdecl _fatal_runtime_error (UINT stk);
    #define FATAL_HANDLER _fatal_runtime_error
  #else
    void cdecl _fatal_runtime_error_ (UINT stk);
    #define FATAL_HANDLER _fatal_runtime_error_
  #endif


  /* Prevent linker (with 'option eliminate') to strip our
   * '_fatal_runtime_error()' function from .exe-image.
   */
  char *dummy_fatal_rte = (char*)&FATAL_HANDLER;

#elif defined(_MSC_VER) && defined(__LARGE__) && defined(USE_DEBUG)
  extern void (__cdecl *_aaltstkovr) (void);
  static void stk_overflow (void _far *where);
#endif

#if (DOSX & (PHARLAP|X32VM)) || (DOSX == 0)
#define CS_WRITEABLE
#endif

#define MAKE_CS_WRITEABLE() ((void)0)  /*!< \todo Make CS writeable */
#define UNDO_CS_ACCESS()    ((void)0)  /*!< \todo Make CS read-only */


#if defined(USE_DEBUG) && defined(__BORLANDC__) && (defined(__SMALL__) || defined(__LARGE__))
STATIC void test_stk_check (void)
{
  char buf[1000];
  sprintf (buf, "In test_stk_check(): CS:IP %04X:%04X\n",
           _CS, FP_OFF(test_stk_check));
  puts (buf);
}
#endif

/*
 * Turn off stack-checking to avoid destroying assumptions
 * made in bswap patch code below. And also to make this run
 * a bit faster.
 */
#include "nochkstk.h"

#if !defined(USE_BIGENDIAN)
/**
 * Convert 32-bit big-endian (network order) to intel (host order) format.
 * Or vice-versa. These are cdecl incase we patch them.
 */
unsigned long cdecl _w32_intel (unsigned long val)
{
  return ((val & 0x000000FFU) << 24) |
         ((val & 0x0000FF00U) <<  8) |
         ((val & 0x00FF0000U) >>  8) |
         ((val & 0xFF000000U) >> 24);
}

/**
 * Convert 16-bit big-endian (network order) to intel (host order) format.
 * Or vice-versa
 */
unsigned short cdecl _w32_intel16 (unsigned short val)
{
  return ((val & 0x00FF) << 8) | ((val & 0xFF00) >> 8);
}

#if (DOSX) && defined(CS_WRITEABLE)
static const BYTE bswap32[] = {
             0x8B,0x44,0x24,0x04,       /* mov eax,[esp+4] */
             0x0F,0xC8,                 /* bswap eax       */
             0xC3                       /* ret             */
           };

static const BYTE bswap16[] = {
             0x8B,0x44,0x24,0x04,       /* mov eax,[esp+4] */
             0x0F,0xC8,                 /* bswap eax       */
             0xC1,0xE8,0x10,            /* shr eax,16      */
             0xC3                       /* ret             */
           };

/*
 * Modify functions intel/intel16 (htonl/htons) to use the
 * BSWAP instruction on 80486+ CPUs. We don't bother with real-mode
 * targets on a 80486+ CPU. Hope that size of overwritten functions
 * are big enough.
 */
static void patch_with_bswap (void)
{
  MAKE_CS_WRITEABLE();  /* save descriptor access, make RW */
  memcpy ((void*)_w32_intel,  (const void*)&bswap32, sizeof(bswap32));
  memcpy ((void*)_w32_intel16,(const void*)&bswap16, sizeof(bswap16));
  UNDO_CS_ACCESS();     /* set old descriptor access */
}
#endif  /* (DOSX) && defined(CS_WRITEABLE) */
#endif  /* !USE_BIGENDIAN */


/**
 * Simplified method of getting days since 1970-01-01.
 * Don't use time() since it will probably link in sprintf().
 * We don't need accuracy. Only a reference for sock_stats().
 */
DWORD get_day_num (void)
{
#if defined(WIN32)
  time_t now;

  time (&now);
  return (now / (24*3600));
#else
  struct dosdate_t d;

  memset (&d, 0, sizeof(d));
  _dos_getdate (&d);
  --d.month;
  --d.day;
  return ((d.year-1970) * 365 + d.month * 31 + d.day);
#endif
}


#if (DOSX) && !defined(USE_BIGENDIAN)
/*
 * Safe check for an enabled RDTSC instruction.
 * Requires an "GenuineIntel" Pentium CPU to call Get_CR4()
 * (crashes on AMD K6-2 etc.).
 */
static BOOL RDTSC_enabled (void)
{
  const char *env = getenv ("USE_RDTSC");

  use_rdtsc = (env && ATOI(env) > 0);

  if (!use_rdtsc ||     /* Usage not enabled */
      x86_type < 5)     /* Not a Pentium class CPU */
     return (FALSE);

  if (!strncmp(x86_vendor_id,"AuthenticAMD",12) &&
      (x86_capability & X86_CAPA_TSC))            /* AMD with TSC, okay? */
     return (TRUE);

  if (!strncmp(x86_vendor_id,"CentaurHauls",12))  /* Centaur/VIA is okay */
  {
#if 0
   /* The following code was originally written by
    * Michal Ludvig <michal@logix.cz> for VIA PadLock code
    * in OpenSSL. http://www.logix.cz/michal
    */
    DWORD eax, ebx, ecx, edx;

    get_cpuid (0xC0000000, &eax, &ebx, &ecx, &edx);
    centaur_eflag = (eax >= 0xC0000001);
    if (centaur_eflag)
    {
      get_cpuid (0xC0000001, &eax, &ebx, &ecx, &edx);
      use_ace = ((edx & (0x3<<6)) == (0x3<<6)); /* Advanced Cryptography Engine */
      use_rng = ((edx & (0x3<<2)) == (0x3<<2)); /* Random Number Generator */
    }
#endif
    return (TRUE);
  }

  if (strncmp(x86_vendor_id,"GenuineIntel",12) || /* Not Genuine Intel or */
      (x86_capability & X86_CAPA_TSC) == 0)       /* RDTSC not supported */
     return (FALSE);

#if (DOSX) && !defined(__LCC__) && !defined(BORLAND_WIN32)
  return ((Get_CR4() & CR4_TS_DISABLE) == 0);     /* True if not disabled */
#else
  return (TRUE);   /* RDTSC never disabled in real-mode */
#endif
}
#endif  /* DOSX && !USE_BIGENDIAN */


#if (DOSX == 0) && defined(USE_DEBUG)
/**
 * Check size of register structs.
 *
 * Some paranoia checks for real-mode targets; if our IREGS structure
 * doesn't match REGPACK of the C-lib, intr() will probably cause a
 * crash.
 */
#define OffsetOf(x) (unsigned)&(x)

static BOOL check_reg_struct (void)
{
#if defined(__WATCOMC__)
  struct IREGS   *r1 = NULL;
  union  REGPACK *r2 = NULL;

  if ((OffsetOf(r2->w.ax)    != OffsetOf(r1->r_ax)) ||
      (OffsetOf(r2->w.bx)    != OffsetOf(r1->r_bx)) ||
      (OffsetOf(r2->w.cx)    != OffsetOf(r1->r_cx)) ||
      (OffsetOf(r2->w.dx)    != OffsetOf(r1->r_dx)) ||
      (OffsetOf(r2->w.bp)    != OffsetOf(r1->r_bp)) ||
      (OffsetOf(r2->w.si)    != OffsetOf(r1->r_si)) ||
      (OffsetOf(r2->w.di)    != OffsetOf(r1->r_di)) ||
      (OffsetOf(r2->w.ds)    != OffsetOf(r1->r_ds)) ||
      (OffsetOf(r2->w.es)    != OffsetOf(r1->r_es)) ||
      (OffsetOf(r2->x.flags) != OffsetOf(r1->r_flags)))
     return (FALSE);

#elif defined(__BORLANDC__)
  struct IREGS   *r1 = NULL;
  struct REGPACK *r2 = NULL;

  if ((OffsetOf(r2->r_ax)    != OffsetOf(r1->r_ax)) ||
      (OffsetOf(r2->r_bx)    != OffsetOf(r1->r_bx)) ||
      (OffsetOf(r2->r_cx)    != OffsetOf(r1->r_cx)) ||
      (OffsetOf(r2->r_dx)    != OffsetOf(r1->r_dx)) ||
      (OffsetOf(r2->r_bp)    != OffsetOf(r1->r_bp)) ||
      (OffsetOf(r2->r_si)    != OffsetOf(r1->r_si)) ||
      (OffsetOf(r2->r_di)    != OffsetOf(r1->r_di)) ||
      (OffsetOf(r2->r_ds)    != OffsetOf(r1->r_ds)) ||
      (OffsetOf(r2->r_es)    != OffsetOf(r1->r_es)) ||
      (OffsetOf(r2->r_flags) != OffsetOf(r1->r_flags)))
     return (FALSE);
#endif
  return (TRUE);
}
#endif /* (DOSX == 0) && USE_DEBUG */

/**
 * Initialise various stuff.
 */
void init_misc (void)
{
  static BOOL init = FALSE;

  if (init)
     return;

  _watt_assert_buf[0] = '\0';

#if (DOSX)
  is_in_stack_init();
#endif

#if defined(WIN32)
  init_winmisc();

#elif defined(__DJGPP__)
  _watt_os_ver = _get_dos_version (1);

#elif defined(MSC386)
  _watt_os_ver = 0x500;   /* Fake it for 32-bit MSVC */

#else
  _watt_os_ver = (_osmajor << 8) + _osminor;
#endif

#if defined(WIN32)
  win32_dos_box = FALSE;
#else
  win32_dos_box = (_watt_os_ver >= 0x700 || /* DOS 7.x/8.x; Win-9x/ME */
                   _watt_os_ver == 0x501 || /* Borland PowerPak under Win32 */
                   _watt_os_ver == 0x532);  /* DOS 5.50; Win-NT+ */
#endif

#if (DOSX & PHARLAP) && defined(HAS_FP)
  /*
   * For 32-bit compilers with 48-bit far-pointers.
   * `init_misc' MUST be called before `PEEKx()' functions are used.
   */
  FP_SET (_watt_dosFp, 0, SS_DOSMEM);

#elif (DOSX & X32VM)
  _watt_dosFp = MK_FP (_x386_zero_base_selector, 0);

#elif defined(HAS_FP) /* MSVC */
  UNFINISHED();
#endif

#if defined(__DJGPP__) || defined(__HIGHC__)
/* backtrace_init(); */  /** \todo */
#endif

#if (DOSX)
  setup_dos_xfer_buf();  /* A no-op on djgpp+Win32 */

#elif defined(USE_DEBUG)  /* real-mode */
  if (!check_reg_struct())
  {
    outsnl (__FILE__ ": IREGS/REGPACK size mismatch!");
    exit (-1);
  }

  #if defined(_MSC_VER) && defined(__LARGE__)
  (DWORD)_aaltstkovr = (DWORD)stk_overflow;
  #endif

  #if defined(__BORLANDC__) && (defined(__SMALL__) || defined(__LARGE__))
  setup_stk_check();
  #endif
#endif

  /* Check CPU type. Use RDTSC instruction if not forced off and
   * it's not disabled. No CPU detection of big-endian machines yet.
   */
#if (DOSX) && !defined(USE_BIGENDIAN)
#if defined(__LCC__)
  if (_cpuidPresent())
  {
    _cpuid (&lcc_cpuid);
    memcpy (&x86_vendor_id, &lcc_cpuid.Vendor, sizeof(x86_vendor_id));
    x86_capability = X86_CAPA_TSC; /* !! this doesn't work: *(DWORD*) &lcc_cpuid.FpuPresent; */
  }
#elif !defined(BORLAND_WIN32)
  CheckCpuType();
#endif

  if (RDTSC_enabled())  /* Try to use RDTSC */
     has_rdtsc = TRUE;

#if defined(CS_WRITEABLE)
  if (x86_type >= 4)
     patch_with_bswap();
#endif
#endif  /* DOSX && !USE_BIGENDIAN */

#if defined(WIN32)
  srand (GetTickCount()); /* should be redundant */
#else
  srand (PEEKW(0,0x46C)); /* initialize rand() using BIOS clock */
#endif

  init_timers();
  init = TRUE;
}

/**
 * WIN32: Open an existing file (or create) in share-mode but deny other
 *   processes to write to the file. On Watcom, fopen() already seems to
 *   open with SH_DENYWR internally.
 * DOS/Watcom: simply call fopen().
 */
FILE *fopen_excl (const char *file, const char *mode)
{
#ifndef _O_SEQUENTIAL    /* bcc32 */
#define _O_SEQUENTIAL 0
#endif

#if defined(WIN32) && !defined(__WATCOMC__)
  int fd = _sopen (file, _O_CREAT | _O_TRUNC | _O_WRONLY | _O_SEQUENTIAL,
                   SH_DENYWR, S_IREAD | S_IWRITE);
  if (fd <= -1)
     return (NULL);
  return fdopen (fd, mode);
#else
  return fopen (file, mode);
#endif
}

/**
 * Register an on-exit function. Insert the 'func' so functions with
 * lowest order are run first. All functions must have different orders.
 */
struct on_exit {
       void      (*func)(void);
       const char *name;
       int         order;
     };

static struct on_exit exit_list [40];

static int list_compare (const struct on_exit *a, const struct on_exit *b)
{
  return (a->order - b->order);
}

/**
 * Register a function to be called from sock_exit().
 */
int rundown_add (void (*func)(void), const char *name, int order,
                 const char *file, unsigned line)
{
  static int num_ins = 0;
  int    i;

  for (i = 0; i < num_ins; i++)
  {
    if (!exit_list[i].func)
       continue;

    if (exit_list[i].func == func)
       goto modify;

    if (exit_list[i].order == order)
    {
#if defined(USE_DEBUG)
      (*_printf) ("%s(%u): rundown_add (\"%s\",%d): order already "
                  "in exit_list[]\n", file, line, name, order);
      exit (-1);
#else
      return (-1);
#endif
    }
  }

  WATT_ASSERT (num_ins < DIM(exit_list));
  i = num_ins++;

modify:
  exit_list[i].func  = func;
  exit_list[i].name  = name;
  exit_list[i].order = order;

  qsort (&exit_list, DIM(exit_list), sizeof(exit_list[0]),
         (int(*)(const void *, const void *))list_compare);

  ARGSUSED (file);
  ARGSUSED (line);
  return (num_ins);
}

/**
 * Run the registered and sorted exit_list[] functions.
 */
void rundown_run (void)
{
  struct on_exit *oe;
  void (*func)(void);
  int  i;

  for (i = 0, oe = exit_list; i < DIM(exit_list); i++, oe++)
  {
    if (!oe->func)
       continue;

#if defined(USE_DEBUG)
    if (debug_on >= 3)
    {
      (*_printf) ("Calling rundown-func `%s' at order %d\n",
                  oe->name, oe->order);
      fflush (stdout);
    }
#endif
     func = oe->func;
     oe->func = NULL;    /* don't call it again */
     (*func)();
  }
}

#if defined(USE_DEBUG)
/**
 * Dump 'exit_list[]' list.
 */
void rundown_dump (void)
{
  const struct on_exit *oe;
  int   i, num_active;

  (*_printf) ("rundown_dump():\n");
  for (i = num_active = 0, oe = exit_list; i < DIM(exit_list); i++, oe++)
  {
    if (!oe->func)
       continue;
    (*_printf) ("  order %3d: %s\n", oe->order, oe->name);
    num_active++;
  }
  (*_printf) ("  %s\n", num_active == i ? "possible overflow" : "okay");
}
#endif


/*
 * Consolidated memory debug for Fortify, mpatrol and MSVC CrtDbg.
 */
#if defined(USE_CRTDBG)
/*
 * If using MSVCRTD debug version, there's little point using Fortify too.
 * USE_CRTDBG is set only when building with cl -MDd or MTd etc (_DEBUG set).
 */
#if !defined(_CRT_RPTHOOK_INSTALL) && (_MSC_VER < 1300)  /* SDK too old */
  #define _CRT_RPTHOOK_INSTALL 0
  #define _CRT_RPTHOOK_REMOVE  1

  _CRTIMP _CRT_REPORT_HOOK __cdecl
   _CrtSetReportHook2 (int, _CRT_REPORT_HOOK);
#endif

static _CrtMemState last_state;
static void __cdecl crtdbg_exit (void);

static const char *report_name (int type)
{
  return (type == _CRT_WARN   ? "Warn"   :
          type == _CRT_ERROR  ? "Error"  :
          type == _CRT_ASSERT ? "Assert" :
          type == _CRT_ERRCNT ? "ErrCnt" : "??");
}

/*
 * This doesn't seem to be called (?)
 */
static void __cdecl crtdbg_dump (const void *buf, size_t len)
{
  const BYTE *p = (const BYTE*) buf;
  size_t i;
  int    c;

  fprintf (stderr, "dump: buf %p, %u bytes\n", buf, len);
  len = min (len, 30);
  for (i = 0; i < len; i++)
  {
    c = *p++;
    fprintf (stderr, "%c", isprint(c) ? c : '.');
  }
}

static int __cdecl crtdbg_report (int type, char *message, int *ret_val)
{
  BOOL stop;

  fprintf (stderr, "%s: %s\n", report_name(type), message);
  stop = (type == _CRT_ASSERT);
  if (stop)
  {
    _CrtMemDumpAllObjectsSince (&last_state);
    crtdbg_exit();
 /* StackWalk (&ret_val); */  /**<\todo make a stack dumper */
  }
  if (ret_val)
     *ret_val = stop;  /* stopping forces a breakpoint (int 3) */
  return (stop);
}

static void __cdecl crtdbg_exit (void)
{
#if 0
  _CrtMemDumpStatistics (&last_state);
  _CrtCheckMemory();
  _CrtDumpMemoryLeaks();
#endif

#if (_MSC_VER >= 1300) && 0
  _CrtSetReportHook2 (_CRT_RPTHOOK_REMOVE, crtdbg_report);
#else
  _CrtSetReportHook (NULL);
#endif
}

void memdbg_init (void)
{
  _HFILE file = _CRTDBG_FILE_STDERR;
  int    mode = _CRTDBG_MODE_FILE;
  int    flags = _CRTDBG_LEAK_CHECK_DF |
                 _CRTDBG_DELAY_FREE_MEM_DF |
              /* _CRTDBG_CHECK_CRT_DF | */    /* Don't report allocs in CRT */
                 _CRTDBG_CHECK_ALWAYS_DF |
                 _CRTDBG_ALLOC_MEM_DF;

  _CrtSetReportFile (_CRT_ASSERT, file);
  _CrtSetReportMode (_CRT_ASSERT, mode);
  _CrtSetReportFile (_CRT_ERROR, file);
  _CrtSetReportMode (_CRT_ERROR, mode);
  _CrtSetReportFile (_CRT_WARN, file);
  _CrtSetReportMode (_CRT_WARN, mode);

  _CrtSetDbgFlag (flags | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));

#if (_MSC_VER >= 1300) && 0
  _CrtSetReportHook2 (_CRT_RPTHOOK_INSTALL, crtdbg_report);
#else
  _CrtSetReportHook (crtdbg_report);
#endif

  _CrtMemCheckpoint (&last_state);
  _CrtSetDumpClient (crtdbg_dump);

  RUNDOWN_ADD (crtdbg_exit, 305);
}

#elif defined(USE_FORTIFY)
/**
 * Report routine for Fortify. Report memory statistics and leaks
 * done in this scope (the core API level). BSD socket API sets
 * it's own scope with reporting done in sock_fortify_exit().
 */
static void fortify_report (void)
{
  if (debug_on >= 2)
  {
    Fortify_SetOutputFunc ((Fortify_OutputFuncPtr)printf);
    Fortify_OutputStatistics();
  }
  else
    Fortify_SetOutputFunc (NULL);
  Fortify_LeaveScope();
}

void memdbg_init (void)
{
  const char *env = getenv ("FORTIFY_FAIL");

  Fortify_EnterScope();
  if (env)
     Fortify_SetAllocateFailRate (atoi(env));

#if defined(USE_DEBUG) && defined(USE_BSD_API)
  Fortify_SetOutputFunc (bsd_fortify_print);
#endif

  RUNDOWN_ADD (fortify_report, 305);
#if defined(WIN32)
  LoadLibrary ("exchndl.dll");  /* Dr. MingW */
#endif
}
#endif   /* USE_CRTDBG */


#if defined(USE_DEBUG) && !defined(NDEBUG)
/**
 * Print an assert fail and store the text for later.
 */
void assert_fail (const char *file, unsigned line, const char *what)
{
#if defined(SNPRINTF)
  SNPRINTF (_watt_assert_buf, sizeof(_watt_assert_buf)-1,
#else
  sprintf (_watt_assert_buf,
#endif
           "%s (%u): Assertion `%s' failed.\n", file, line, what);

  (*_printf) ("%s\n", _watt_assert_buf);

#ifdef __DJGPP__
  /*
   * abort() doesn't call atexit() functions but makes a handy traceback.
   */
  rundown_run();
  abort();
#else
  exit (-1);
#endif
}
#endif  /* USE_DEBUG && !NDEBUG */


/**
 * Returns a random integer in range \b [a..b].
 */
unsigned Random (unsigned a, unsigned b)
{
  if (a == b)
     return (a);

  if (a > b)
  {
    unsigned tmp = b;
    b = a;
    a = tmp;
  }
  return (a + (unsigned)(rand() % (b-a+1)));
}

/**
 * Wait for a random period in range \b [a..b] millisec.
 */
void RandomWait (unsigned a, unsigned b)
{
  DWORD t = set_timeout (Random(a, b));

  while (!chk_timeout(t))
  {
#if defined(WIN32)
    Sleep (1);
#else
    ENABLE();
#endif
  }
}

/**
 * Not all vendors have delay().
 */
void Wait (unsigned msec)
{
  DWORD t = set_timeout (msec);

  while (!chk_timeout(t))
  {
#if defined(WIN32)
    Sleep (1);
#else
    ENABLE();
#endif
  }
}

/*
 * Return a sensible name for running DOS-extender or subsystem.
 */
const char *dos_extender_name (void)
{
#if (DOSX & DOS4GW)
  return dos4gw_extender_name();
#elif (DOSX & DJGPP)
  return ("djgpp");
#elif (DOSX & PHARLAP)
  return ("PharLap");
#elif (DOSX & POWERPAK)
  return ("PowerPak");
#elif (DOSX & X32VM)
  return ("X32VM");
#elif (DOSX & WINWATT)
  return ("Win32");
#else
  return (NULL);
#endif
}


void os_yield (void)
{
#if defined(WIN32)
  /* SwitchToThread(); */ /* This doesn't really help reduce CPU usage */
  Sleep (1);
#else
  static BOOL do_yield = TRUE;

#if defined(__DJGPP__)
  if (do_yield)
  {
    __dpmi_yield();
    do_yield = (errno != ENOSYS);
  }
#else
  if (!watt_kbhit() && do_yield)  /* watt_kbhit() to permit ^C */
  {
    IREGS reg;
    memset (&reg, 0, sizeof(reg));
    reg.r_ax = 0x1680;
    GEN_INTERRUPT (0x2F, &reg);
    do_yield = (loBYTE(reg.r_ax) != 0x80);
  }
#endif
#endif  /* WIN32 */
}


#if defined(NOT_USED)
BOOL watt_check_break (void)
{
  WORD head = 0x400 + PEEKW (0, 0x41A);
  WORD tail = 0x400 + PEEKW (0, 0x41C);
  int  ofs, num;

  printf ("\nwatt_check_break(): head %04X, tail %04X, keys:\n",
          head, tail);
  for (num = 0, ofs = head; ofs != tail && num < 32; num++)
  {
    printf ("  ofs %04X: %02X\n", ofs, PEEKB(0,ofs));
    if (++ofs > 32+0x41E)
       ofs = 0x41E;
  }
  puts ("");
  return (FALSE);
}
#endif


/**
 * A less CPU hogging kbhit().
 */
int watt_kbhit (void)
{
  if (_watt_cbroke)
     return (1);

#if defined(WIN32)
  Sleep (1);
  return kbhit();

#else
  if (PEEKW(0,0x41A) == PEEKW(0,0x41C))
     return (0);

  /* RTL's kbhit() calls INT16/11 or INT21/0B which triggers INT 23
   * which in turn should raise SIGINT in RTL. Except for djgpp under
   * Windows where normal/extended BREAK checking is turned off (since
   * SIGINT delivery by pressing ^C is so unreliable under Windows).
   */
  return kbhit();
#endif
}


#if defined(USE_DEBUG)
/**
 * Search 'list' for 'type' and return it's name.
 */
const char *list_lookup (DWORD type, const struct search_list *list, int num)
{
  static char buf[15];

  while (num > 0 && list->name)
  {
    if (list->type == type)
       return (list->name);
    num--;
    list++;
  }
  sprintf (buf, "?%lu", type);
  return (buf);
}


/**
 * Return hexa-decimal string for an 6/7 byte MAC-address.
 * Use 2 buffers in round-robin.
 */
const char *MAC_address (const void *addr)
{
  static char buf[2][25];
  static int  idx = 0;
  char  *rc = buf [idx];
  char  *p  = rc;
  const char *a = (const char*)addr;

  p += sprintf (p, "%02X:%02X:%02X:%02X:%02X:%02X",
                a[0] & 255, a[1] & 255, a[2] & 255,
                a[3] & 255, a[4] & 255, a[5] & 255);

  /* assume '*addr' is an 7-byte AX25 address
   */
  if (_pktdevclass == PDCLASS_AX25)
     sprintf (p, ":%02X", a[6] & 255);

  idx ^= 1;
  return (rc);
}

void unfinished (const char *func, const char *file, unsigned line)
{
  if (func)
     fprintf (stderr, "In `%s' ", func);
  fprintf (stderr, "%s (%u):\7 Help! Unfinished code.\n", file, line);
  exit (-1);
}

/**
 * Return nicely formatted string "xx,xxx,xxx"
 * with thousand separators (left adjusted).
 */
const char *dword_str (DWORD val)
{
  static char buf[20];
  char   tmp[20];

  if (val < 1000UL)
  {
    sprintf (buf, "%lu", val);
    return (buf);
  }
  if (val < 1000000UL)       /* 1E6 */
  {
    sprintf (buf, "%lu,%03lu", val/1000UL, val % 1000UL);
    return (buf);
  }
  if (val < 1000000000UL)    /* 1E9 */
  {
    sprintf (tmp, "%9lu", val);
    sprintf (buf, "%.3s,%.3s,%.3s", tmp, tmp+3, tmp+6);
    return strltrim (buf);
  }
  sprintf (tmp, "%12lu", val);
  sprintf (buf, "%.3s,%.3s,%.3s,%.3s", tmp, tmp+3, tmp+6, tmp+9);
  return strltrim (buf);
}
#endif /* USE_DEBUG */


#if (DOSX)
#if defined(BORLAND386) || defined(DMC386) || defined(MSC386)
DWORD get_ds_limit (void)
{
  DWORD res;
  asm mov ax, ds
  asm and eax, 0FFFFh
  asm lsl eax, eax
  asm mov res, eax
  return (res);
}

DWORD get_cs_limit (void)
{
  DWORD res;
  asm mov ax, cs
  asm and eax, 0FFFFh
  asm lsl eax, eax
  asm mov res, eax
  return (res);
}

DWORD get_ss_limit (void)
{
  DWORD res;
  asm mov ax, ss
  asm and eax, 0FFFFh
  asm lsl eax, eax
  asm mov res, eax
  return (res);
}

#elif defined(__CCDL__)
DWORD get_ds_limit (void)
{
  asm {
    mov ax, ds
    and eax, 0xFFFF
    lsl eax, eax
  }
  return (_EAX);
}

DWORD get_cs_limit (void)
{
  asm {
    mov ax, cs
    and eax, 0xFFFF
    lsl eax, eax
  }
  return (_EAX);
}

DWORD get_ss_limit (void)
{
  asm {
    mov ax, ss
    and eax, 0xFFFF
    lsl eax, eax
  }
  return (_EAX);
}
#endif

/*
 * The is_in_stack() function is by
 * Jani Kajala (jani.kajala@helsinki.fi)
 * Feb 7, 2002,
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
  #define THREADLOCAL __declspec(thread)
#else
  #define THREADLOCAL
#endif

THREADLOCAL static const char *stack_bottom = 0;

#if defined(__DJGPP__) && 0 /* not needed */
  extern unsigned dj_end asm ("end");
  extern unsigned _stklen, __djgpp_stack_limit;
  #define STK_START()  (DWORD)&dj_end
#endif

static unsigned get_frame_size (const char *x)
{
  char y = 0;
  return (unsigned)(x - &y);
}

static void is_in_stack_init (void)
{
  char x = 0;
  stack_bottom = &x + get_frame_size(&x) * 2;
}

BOOL is_in_stack (const void *ptr)
{
  char  x;
  char *stack_top = &x;
  char *p         = (char*) ptr;

  if (stack_top > stack_bottom)
     return (p > stack_bottom && p < stack_top);  /* stack grows up */
  return (p > stack_top && p < stack_bottom);     /* stack grows down */
}

unsigned used_stack (void)
{
  char  x;
  char *stack_top = &x;

  if (stack_top > stack_bottom)
      return (stack_top - stack_bottom);
  return (stack_bottom - stack_top);
}

/*
 * Test for valid read/write data address.
 * We assume linear address 'addr' is both readable and writeable.
 *
 * \note MingW (and other Win32 compilers?) puts 'const' data in
 *       read-only sections (.rdata). Detectible with IsBadWritePtr().
 */
BOOL valid_addr (DWORD addr, unsigned len)
{
#if defined(WIN32)
  if (IsBadWritePtr((void*)addr,len) && IsBadReadPtr((const void*)addr,len))
     return (FALSE);

#else
  DWORD limit;

  if (addr < 4096)  /* Valid in DOS4GW, but we never uses such address */
     return (FALSE);

 /* In X32VM: DS != SS. addr may be in data or stack.
  */
#if defined(DMC386) && (DOSX & X32VM) && 0  /* Doesn't work :-( */
  if (selector == MY_SS())
  {
    limit = get_ss_limit();
    if (addr < _x386_stacklow || addr + len >= limit)
       return (FALSE);
    if (limit > len && addr >= limit - len)  /* Segment wrap */
       return (FALSE);
  }
  else if (selector != MY_DS())
    return (FALSE);
#endif

#if defined (__DJGPP__)
  limit = __dpmi_get_segment_limit (_my_ds());
#elif defined (__HIGHC__)
  limit = _mwlsl (_mwgetcs());   /* DS & CS are aliases */
#else
  limit = get_ds_limit();
#endif

  if (addr + len >= limit)
     return (FALSE);
  if (limit > len && addr >= limit - len)  /* Segment wrap */
     return (FALSE);
#endif  /* WIN32 */

  return (TRUE);
}
#endif  /* DOSX */


/*
 * Pharlap/X32VM targets:   Get location of (or allocate a) transfer buffer.
 * DOS4GW/PowerPak targets: Allocate a small (1kB) transfer buffer.
 * djgpp/Win32:             Nothing special to do.
 */
#if (DOSX & (PHARLAP|X32VM))
  REALPTR _watt_dosTbr;
  DWORD   _watt_dosTbSize = 0;
  static WORD rm_seg = 0;

  static void free_selector (void)
  {
    if (rm_seg)
       _dx_real_free (rm_seg);
    rm_seg = 0;
    _watt_dosTbr = 0;
    _watt_dosTbSize = 0;
  }

  static void setup_dos_xfer_buf (void)
  {
    FARPTR  dos_tbp;     /* pmode transfer-buffer address */
    REALPTR r2p_addr;    /* rmode to pmode call address */
    WORD    temp;
    int     len = 1024;  /* min size we need */

    _dx_rmlink_get (&r2p_addr, &_watt_dosTbr,
                    &_watt_dosTbSize, &dos_tbp);

    if (_watt_dosTbSize < len &&
        _dx_real_above(len, &rm_seg, &temp) == 0)
    {
      RP_SET (_watt_dosTbr, 0, rm_seg);
      _watt_dosTbSize = len;
      RUNDOWN_ADD (free_selector, 15);
    }
  }

#elif (DOSX & DOS4GW)
  WORD  _watt_dosTbSeg  = 0;   /* paragraph address of xfer buffer */
  WORD  _watt_dosTbSel  = 0;   /* selector for transfer buffer */
  DWORD _watt_dosTbSize = 0;   /* size of transfer buffer */

  static void free_selector (void)
  {
    if (_watt_dosTbSel)
  #ifdef __CCDL__
       dpmi_free_selector (_watt_dosTbSel);
  #else
       dpmi_real_free (_watt_dosTbSel);
  #endif
    _watt_dosTbSel = 0;
  }

  static void setup_dos_xfer_buf (void)
  {
    _watt_dosTbSize = 1024;
  #ifdef __CCDL__
    if (dpmi_alloc_real_memory (&_watt_dosTbSel, &_watt_dosTbSeg, _watt_dosTbSize) >= 0)
       _watt_dosTbSeg = 0;
  #else
    _watt_dosTbSeg = dpmi_real_malloc (_watt_dosTbSize, &_watt_dosTbSel);
  #endif

    if (!_watt_dosTbSeg)
         _watt_dosTbSize = 0;
    else RUNDOWN_ADD (free_selector, 15);
  }

#elif (DOSX & POWERPAK)
  WORD  _watt_dos_ds;
  WORD  _watt_dosTbSeg  = 0;
  WORD  _watt_dosTbSel  = 0;
  DWORD _watt_dosTbr    = 0;
  DWORD _watt_dosTbSize = 0;

  static void free_selector (void)
  {
    if (_watt_dosTbSel)
       dpmi_real_free (_watt_dosTbSel);
    _watt_dosTbSel = 0;
    if (_watt_dos_ds)
       dpmi_free_dos_selector (_watt_dos_ds);
    _watt_dos_ds = 0;
    _watt_dosTbr = 0;
  }

  static void setup_dos_xfer_buf (void)
  {
    _watt_dos_ds = dpmi_create_dos_selector();
    if (!_watt_dos_ds)
    {
      fprintf (stderr, "Fatal: Failed to create DOS selector. "
               "DPMI error 0x%04X.\n", __dpmi_errno);
      exit (-1);
    }
    _watt_dosTbSize = 1024;
    _watt_dosTbSeg  = dpmi_real_malloc (_watt_dosTbSize, &_watt_dosTbSel);
    if (!_watt_dosTbSeg)
       _watt_dosTbSize = 0;
    else
    {
      RUNDOWN_ADD (free_selector, 15);
      _watt_dosTbr = (_watt_dosTbSeg << 16);
    }
  }

#elif (DOSX)
  static void setup_dos_xfer_buf (void)
  {
    /* no-op */
  }
#endif


#if defined(USE_BSD_API)
/*
 * ffs() isn't needed yet, but could be used in select_s()
 *
 * Copyright (C) 1991, 1992 Free Software Foundation, Inc.
 * Contributed by Torbjorn Granlund (tege@sics.se).
 *
 * The GNU C Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The GNU C Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the GNU C Library; see the file COPYING.LIB.  If
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA.
 */

/*
 * Returns the index of first bit set in 'i'. Counting from 1 at
 * "right side". Returns 0 if 'i' is 0.
 */
int _w32_ffs (int i)
{
  static BYTE table[] = {
    0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
  };
  DWORD a, x;

#if (DOSX) && !defined(__LCC__) && !defined(BORLAND_WIN32)
  if (x86_type >= 3)
     return asm_ffs (i);  /* BSF requires 386+ */
#endif

  x = i & -i;
  a = x <= 0xFFFFUL ? (x <= 0xFF ? 0 : 8) : (x <= 0xFFFFFF ? 16 : 24);
  return (table[x >> a] + a);
}
#endif  /* USE_BSD_API */


/*
 * Checks for bugs when compiling in large model C compiler
 *
 * Borland C uses a 4K stack by default. In all memory models the
 * stack grows down toward the heap.
 *
 * If you accidentally place _tcp_Socket onto the stack, then you
 * will have already used up that whole 4K and then some!
 *
 * In large model, this will mess up the data space in a major way
 * because the stack starts at SS:_stklen, or SS:1000, so you will
 * wrap the SP pointer back around to FFFE and start writing over
 * the far heap.  Yuck.
 *
 * In small model it usually doesn't kill your application because
 * you would have to be down to your last 4K of memory and this is
 * not as common.
 *
 * The solutions: declare your sockets as static, or put them on the
 * heap, or bump up your stack size by using the global special variable:
 *
 * unsigned _stklen = 16536;
 */

#if defined(__LARGE__)
void watt_large_check (const void *sock, int size,
                       const char *file, unsigned line)
{
  if ((unsigned)(FP_OFF(sock)) > (unsigned)(-size))
  {
#if defined(USE_DEBUG)
    fprintf (stderr, "%s (%d): user stack size error", file, line);
#else
    outsnl ("user stack size error");
    ARGSUSED (file);
    ARGSUSED (line);
#endif
    exit (3);
  }
}
#endif


#if defined(USE_DEBUG)
#if defined(NOT_USED)
int isbigendian (void)
{
 /* From Harbison & Steele.
  */
  union {
    long l;
    char c[sizeof(long)];
  } u;

  u.l = 1;
  return (u.c[sizeof(long)-1] == 1);
}

/**
 * Run a COMMAND.COM/4DOS external/internal command
 */
#include <sys/packon.h>

struct cmd_block {
       BYTE len;
       char buf[256];
     };

#include <sys/packoff.h>

BOOL shell_exec (const char *cmd)
{
  struct cmd_block blk;
  size_t i = sizeof(blk.buf)-1;
  IREGS  regs;

  StrLcpy (blk.buf, cmd, i);
  i = min (i, strlen(blk.buf));
  blk.len    = i;
  blk.buf[i] = '\r';

#if (DOSX & DJGPP)
  regs.r_ds = __tb / 16;
  regs.r_si = __tb & 15;
  dosmemput (&blk, sizeof(blk), __tb);

#elif (DOSX == 0)
  regs.r_ds = FP_SEG (&blk);
  regs.r_si = FP_OFF (&blk);
#endif

  GEN_INTERRUPT (0x2E, &regs);
  return (regs.r_ax != 0xFFFF);
}
#endif  /* NOT_USED */


#if defined(TEST_PROG)
/*
 * Get/set DOS' memory allocation strategy.
 */
BOOL get_mem_strat (BYTE *strat)
{
  IREGS reg;

  memset (&reg, 0, sizeof(reg));
  reg.r_ax = 0x5800;
  GEN_INTERRUPT (0x21, &reg);
  if (reg.r_flags & CARRY_BIT)
     return (FALSE);
  *strat = loBYTE (reg.r_ax);
  return (TRUE);
}

BOOL set_mem_strat (BYTE strat)
{
  IREGS reg;

  memset (&reg, 0, sizeof(reg));
  reg.r_ax = 0x5801;
  reg.r_bx = strat;
  GEN_INTERRUPT (0x21, &reg);
  if (reg.r_flags & CARRY_BIT)
     return (FALSE);
  return (TRUE);
}
#endif  /* TEST_PROG */

/**
 * Microsoft/Digital Mars doesn't have intr() so we make our own.
 */
#if (defined(_MSC_VER) || defined(__DMC__)) && (DOSX == 0)
#undef intr
void _w32_intr (int int_no, IREGS *reg)
{
  union  REGS  r;
  struct SREGS s;

  r.x.ax = reg->r_ax;
  r.x.bx = reg->r_bx;
  r.x.cx = reg->r_cx;
  r.x.dx = reg->r_dx;
  r.x.si = reg->r_si;
  r.x.di = reg->r_di;
  s.ds   = reg->r_ds;
  s.es   = reg->r_es;
  int86x (int_no, &r, &r, &s);
  reg->r_flags = r.x.cflag;
  reg->r_ax    = r.x.ax;
  reg->r_bx    = r.x.bx;
  reg->r_cx    = r.x.cx;
  reg->r_dx    = r.x.dx;
  reg->r_si    = r.x.si;
  reg->r_di    = r.x.di;
  reg->r_ds    = s.ds;
  reg->r_es    = s.es;
}
#endif


#if defined(__BORLANDC__) && defined(__LARGE__)
/*
 * Large model with option '-N' generates code like:
 *                      < .. standard prologue ...>
 *   39 26 00 00        cmp       DGROUP:__stklen,sp
 *   77 05              ja        Lxx
 *   9A 00 00 00 00     call      F_OVERFLOW@
 *                    Lxx:
 *
 * We need to find the address of F_OVERFLOW@ (it cannot be addressed
 * from C).
 */
STATIC void stk_overflow (WORD ret_addr)
{
  static WORD cs, ip;
  static WORD stk[128];

  cs = *(WORD*) (&ret_addr-1);
  ip = *(WORD*) (&ret_addr-2);

  _SS = FP_SEG (stk);
  _SP = (WORD) &stk [DIM(stk)-1];
  _stklen = 0x7FFF;
  _watt_fatal_error = TRUE;
  fprintf (stderr, "\nStack overflow at %04X:%04X!\n", cs, ip);
  _eth_release();
  _exit (-1);
}

STATIC int setup_stk_check (void)
{
  BYTE  sign[] = { 0x39, 0x26, 0,0, 0x77, 5, 0x9A };
  BYTE *p      = (BYTE*) _eth_arrived;
  int   i;

  *(WORD*)&sign[2] = FP_OFF (&_stklen);
  for (i = 0; i < 10; i++, p++)
  {
      if (!memcmp(p, sign, sizeof(sign)))
      {
        DWORD addr = *(DWORD*) (p + sizeof(sign));
        BYTE *patch;

     /* printf ("F_OVERFLOW@ at %04X:%04X\n", (WORD)(addr >> 16), (WORD)addr); */
        patch = (BYTE*) addr;
        *patch++ = 0x2E;
        *patch++ = 0xFF;
        *patch++ = 0x2E;
        *(WORD*)patch = 6 - 1 + (WORD)addr;  /* relocation of [where] */
        patch += 2;
        *(WORD*)patch = FP_OFF (stk_overflow);
        patch += 2;
        *(WORD*)patch = FP_SEG (stk_overflow);

        /*
         *  0001                    F_OVERFLOW@:
         *  0001  2E: FF 2E 0006r     jmp dword ptr cs:[where]
         *  0006  ????????            where dd ?
         */
#if 0
        printf ("test_stk_check() at CS:IP %04X:%04X\n",
                _CS, FP_OFF(test_stk_check));
        _stklen = 10;
        test_stk_check();
#endif
        return (1);
      }
  }
  return (0);
}

#elif defined(__BORLANDC__) && defined(__SMALL__)
/*
 * Small model with option '-N' generates code like:
 *                      < .. standard prologue ...>
 *   39 26 00 00        cmp       ___brklvl,sp
 *   72 03              jb        L$1
 *   E8 00 00           call      N_OVERFLOW@
 *                    L$1:
 *
 * We need to find the address of N_OVERFLOW@ (it cannot be addressed
 * from C). It should be the same code as in large model.
 */
extern int __brklvl;

static void stk_overflow (UINT ret_addr)
{
  static WORD cs, ip;
  static WORD stk[128];

  cs = _CS;
  ip = *(WORD*) (&ret_addr-1);

  _SS = FP_SEG (stk);
  _SP = (WORD) &stk [DIM(stk)-1];

  _stklen = 0x7FFF;
  _watt_fatal_error = TRUE;
  fprintf (stderr, "\nStack overflow at %04X:%04X!\n", cs, ip);
  _eth_release();
  _exit (1);     /* do minimal work, no rundown_run() */
}

static int stk_check_setup (void)
{
  BYTE  sign[] = { 0x39, 0x26, 0,0, 0x72, 3, 0xE8 };
  BYTE *p      = (BYTE*) _eth_arrived;
  int   i;

  *(WORD*)&sign[2] = FP_OFF (&__brklvl);
  for (i = 0; i < 10; i++, p++)
      if (!memcmp(p, sign, sizeof(sign)))
      {
        BYTE *addr = *(BYTE*)(++p);
        *addr = (BYTE*) stk_overflow;
        return (1);
      }
  return (0);
}

#elif defined(WATCOM386) && !defined(WIN32)
/*
 * For tracking down stack overflow bugs.
 * Stack checker __CHK is pascal-style with stack-size at [esp+4].
 * __CHK calls __STK which in turn may call _fatal_runtime_error()
 *
 * Compiling with stack-checking on, this prologue is in every function:
 *  (*) push <stack size needed>   <- 68h, dword size at EIP-9
 *      call __CHK                 <- 5 bytes
 *      ...                        <- extracted EIP of return
 */
static void stk_overflow (WORD cs, UINT eip)
{
  UINT size = *(UINT*)(eip-9);

  eip -= (UINT)&__begtext - 9; /* print .map-file address of (*) */

  _watt_fatal_error = TRUE;
  fprintf (stderr, "Stack overflow (%u bytes needed) detected at %X:%08lXh\n",
           size, cs, (DWORD)eip);
  _eth_release();
  _exit (1);     /* do minimal work, no rundown_run() */
}

void FATAL_HANDLER (UINT stk)
{
#if defined(__SMALL__)
  _x386_stacklow = stk + 2;
  stk_overflow (MY_CS(), *(DWORD*)(&stk+1));

#elif defined(__LARGE__)
  _x386_stacklow = stk + 4;
  stk_overflow (*(WORD*)(&stk+1), *(WORD*)(&stk+2)); /* far-call */

#else     /* wcc386/DOS */
  _x386_stacklow = stk + 4;
  stk_overflow (MY_CS(), *(WORD*)(&stk+1));
#endif
}

#elif defined(_MSC_VER) && defined(__LARGE__)
static void stk_overflow (void _far *where)
{
  fprintf (stderr, "Stack overflow detected at %04X:%04Xh\n",
           FP_SEG(where), FP_OFF(where));
  _eth_release();
  _exit (1);
}
#endif
#endif  /* USE_DEBUG */

/*
 * Functions needed for "gcc -O0". These are inlined on -O1 or above.
 */
#if defined(__GNUC__)
#undef __SYS_SWAP_BYTES_H
#undef _w32_SYS_SWAP_BYTES_H
#undef _w32_CPUMODEL_H
#undef _w32_MISC_H
#undef _w32_IOPORT_H
#undef BEEP
#undef WIN_ASSERT
#undef _W32_EXTERN_INLINE

#define extern
#define __inline__
#define __inline
#define _W32_EXTERN_INLINE

#if defined(__MINGW32__) || defined(__CYGWIN__)
  #undef intel
  #undef intel16
  #define __NO_INLINE__  /* emulate -O0 */
#endif

#include <sys/swap.h>
#include "misc.h"
#include "cpumodel.h"

#if defined(__EMX__)
#include "ioport.h"
#endif

#endif  /* __GNUC__ */


#if defined(TEST_PROG)

void foo_10 (void) { puts ("I'm foo_10()"); }
void foo_20 (void) { puts ("I'm foo_20()"); }
void foo_30 (void) { puts ("I'm foo_30()"); }
void foo_40 (void) { puts ("I'm foo_40()"); }
void foo_50 (void) { puts ("I'm foo_50()"); }
void foo_60 (void) { puts ("I'm foo_60()"); }
void foo_70 (void) { puts ("I'm foo_70()"); }

int main (void)
{
  BYTE strat = 0;

  printf ("DOS memory allocation strategy: ");
  if (!get_mem_strat(&strat))
       puts ("failed");
  else printf ("0x%02X\n", strat);

  printf ("Setting \"low memory best fit\" ");
  if (!set_mem_strat(1))
       puts ("failed");
  else puts ("okay");

  set_mem_strat (strat);

  RUNDOWN_ADD (foo_40, 40);
  RUNDOWN_ADD (foo_10, 10);
  RUNDOWN_ADD (foo_60, 60);
  RUNDOWN_ADD (foo_30, 30);
  RUNDOWN_ADD (foo_20, 20);
  RUNDOWN_ADD (foo_50, 50);
  RUNDOWN_ADD (foo_70, 70);
  debug_on = 2;
  rundown_run();
  return (0);
}
#endif

