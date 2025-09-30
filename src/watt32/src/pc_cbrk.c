/*!\file pc_cbrk.c
 *
 * Ctrl-C, Ctrl-Break handling.
 */
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>

#include "copyrigh.h"
#include "wattcp.h"
#include "sock_ini.h"
#include "wdpmi.h"
#include "x32vm.h"
#include "misc.h"
#include "printk.h"
#include "bsddbug.h"
#include "language.h"
#include "strings.h"

#ifdef __DJGPP__
#include <sys/exceptn.h>
#endif

#if defined(__HIGHC__)
  extern UINT _mwstack_limit[2]; /* stack-check limits */
  pragma Alias (_mwstack_limit, "_mwstack_limit");
  #undef SIGBREAK        /* SIGBREAK == SIGQUIT */

#elif defined(__DMC__)   /* ditto */
  #undef SIGBREAK
#endif

#ifdef TEST_PROG
  #include <conio.h>
  #undef  outsnl
  #define outsnl(str)  cputs (str)
#endif

/**
 * Turn off stack-checking here because stack may be out of bounds.
 * longjmp() hopefully fixes that.
 */
#include "nochkstk.h"

WORD         _watt_handle_cbreak = 0; /*!< changes operation of the break stuff */
volatile int _watt_cbroke        = 0; /*!< increment on SIGINT catch */

static int cbreak_mode;

#if !defined(WIN32)
static jmp_buf sig_jmp;
#endif

/**
 * SIGINT/SIGBREAK handler function.
 */
void MS_CDECL sig_handler_watt (int sig)
{
#ifdef TEST_PROG
  BEEP();
#endif

  if (_watt_handle_cbreak) /* inside _arp_resolve(), lookup_domain() etc. */
  {
    _watt_cbroke++;
    if (cbreak_mode & 0x10)
       outsnl (_LANG("\nInterrupting"));
  }
  else if (cbreak_mode & 1)
  {
    outsnl (_LANG("\nBreaks ignored"));
  }
  else
  {
#if defined(__HIGHC__)
    static UINT new_stk [1024];
    _mwstack_limit[1] = (UINT)&new_stk;  /* set new Bottom-Of-Stack */
#elif defined(__DJGPP__)
    signal (sig, SIG_IGN);               /* ignore until we quit */
#endif

    _watt_cbroke++;

#if defined(WIN32)
    return;
#else
    longjmp (sig_jmp, sig);
#endif
  }

  signal (sig, sig_handler_watt);        /* rearm our handler */

  /* !! Seems the BREAK state of NTVDM gets reenabled when pressing ^C.
   * So disable the state again (not reentrant)
   */
  if (sig == SIGINT && (cbreak_mode & 1))
     set_cbreak (0);
}


/**
 * Sets normal and extended BREAK mode.
 * \arg want_brk 0x0000: no ^BREAK checking.
 * \arg want_brk 0x0001: normal ^BREAK checks.
 * \arg want_brk 0x0101: extended ^BREAK checks.
 *
 * \retval normal BREAK state in LSB, extended BREAK state in MSB.
 */
int set_cbreak (int want_brk)
{
#if defined(WIN32)
  ARGSUSED (want_brk);
  return (0);
#else
  IREGS reg;
  WORD  brk;

#if 0
 /*
  * After much testing (ref .\tests\pc_cbrk.exe), I found the following
  * to be most reliable for both DOS and Windows-NT. Using any combination
  * of `cbreak_mode' and pressing ^C seems to work in Win-NT, but ^Break still
  * have problems...
  */
  __djgpp_set_ctrl_c (want_brk & 1);

 /* don't count ^Breaks; the 0x1B vector isn't always restored
  * at program exit
  */
  _go32_want_ctrl_break (want_brk & 1);
#endif

  reg.r_ax = 0x3300;             /* get normal BREAK state */
  GEN_INTERRUPT (0x21, &reg);
  brk = loBYTE (reg.r_dx);

#if 0
  if (want_brk & 0x0100)
  {
    reg.r_ax = 0x3302;
    GEN_INTERRUPT (0x21, &reg);
    brk |= loBYTE(reg.r_dx) << 8;
  }
#endif

  if (want_brk == 0)
  {
    reg.r_ax = 0x3301;          /* set normal BREAK state off */
    reg.r_dx = 0;
    GEN_INTERRUPT (0x21, &reg);
    reg.r_ax = 0x3302;          /* set extended BREAK state off */
    reg.r_dx = 0;
    GEN_INTERRUPT (0x21, &reg);
  }
  else                    /* set normal/extended BREAK state on */
  {
    reg.r_ax = (want_brk >= 0x0100) ? 0x3302 : 0x3301;
    reg.r_dx = 1;
    GEN_INTERRUPT (0x21, &reg);
  }
  return (brk);
#endif  /* WIN32 */
}

/**
 * Sets our break mode.
 * \arg `mode' is composed of the following flags
 *   - 0x01: disallow breakouts
 *   - 0x10: display a message upon ^C/^BREAK (default)
 */
int tcp_cbreak (int mode)
{
  volatile int rc = 0;

  signal (SIGINT, sig_handler_watt);

#if defined(SIGBREAK)
  signal (SIGBREAK, sig_handler_watt);
#endif

#if defined(__DJGPP__)
  if (win32_dos_box)
     rc = set_cbreak (0);  /* ignore DOS BREAK checking */
  else
#endif
     rc = set_cbreak (!(mode & 1));

  cbreak_mode = mode;

#if !defined(WIN32)
  {
    int sig;

    /* Some vendors calls signal-handlers with a very limited stack.
     * We longjmp() out of it to restore to a safe stack.
     */
    sig = setjmp (sig_jmp);
    if (sig)
       sock_sig_exit ("\nTerminating.", sig);
  }
#endif
  return (rc);
}

#if defined(TEST_PROG)

void usage (char *argv0)
{
  printf ("Usage: %s normal | nobrk | graceful\n", argv0);
  exit (-1);
}

int old_brk = -1;

int main (int argc, char **argv)
{
  int mode = 0, show_brk = 1;

  if (argc != 2)
     usage (argv[0]);

  if (!stricmp(argv[1],"normal"))
  {
    mode = 0x10;
    _watt_handle_cbreak = 0;
  }
  else if (!stricmp(argv[1],"nobrk"))
  {
    mode = 0x01;
    _watt_handle_cbreak = 1;
    puts ("Press <^BREAK> three times to exit");
  }
  else if (!stricmp(argv[1],"graceful"))
  {
    mode = 0x00;
    _watt_handle_cbreak = 1;
    puts ("Press <^C> or <^BREAK> three times to exit");
  }
  else
    usage (argv[0]);

  if (show_brk)
     mode |= 0x10;

  old_brk = tcp_cbreak (mode);
  _watt_cbroke = 0;

  while (_watt_cbroke < 3)
  {
    usleep (200000);

    /* PEEKB on djgpp changes the FS register. This is just to
     * test if SIGINT handler restores FS/GS registers.
     */
    (void) PEEKB (0, 0);
    kbhit();
    putchar ('.');
    fflush (stdout);
  }
  printf ("`_watt_cbroke' set %d times\n", _watt_cbroke);
  set_cbreak (old_brk);
  return (1);
}
#endif /* TEST_PROG */
