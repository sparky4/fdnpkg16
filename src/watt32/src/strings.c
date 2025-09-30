/*!\file strings.c
 *
 * Printing to console and general string handling.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "wattcp.h"
#include "misc.h"
#include "strings.h"

/*
 * This file contains some gross hacks. Take great care !
 */
#include "nochkstk.h"

#ifdef __WATCOMC__
  extern void _outchar (char c);
  #pragma aux _outchar = \
          "mov ah, 2"    \
          "int 21h"      \
          parm [dl]      \
          modify [ax];
#endif

/*
 * Print a single character to stdout.
 */
static void outch (char chr)
{
  if (chr == (char)0x1A)     /* EOF (^Z) causes trouble to stdout */
     return;

#if defined(WIN32)
  {
    HANDLE hnd = GetStdHandle (STD_OUTPUT_HANDLE);

    if (hnd != INVALID_HANDLE_VALUE)
       WriteConsoleA (hnd, &chr, 1, NULL, NULL);
  }
#elif !(DOSX)
  /* Qemm doesn't like INT 21 without a valid ES reg. intdos() sets up that.
   */
  {
    union REGS r;
    r.h.ah = 2;
    r.h.dl = chr;
    intdos (&r, &r);
  }

#elif (DOSX & POWERPAK)
  {
    union REGS r;
    r.h.ah = 2;
    r.h.dl = chr;
    int386 (0x21, &r, &r);
  }

#elif defined(__BORLANDC__)  /* avoid spawning tasm.exe */
  _ES = _DS;
  _DL = chr;
  _AL = 2;
  geninterrupt (0x21);

#elif defined(_MSC_VER) || defined(__DMC__)
  asm mov dl, chr
  asm mov ah, 2
  asm int 21h

#elif defined(__CCDL__)
  asm mov dl, [chr]
  asm mov ah, 2
  asm int 0x21

#elif defined(__WATCOMC__)
  _outchar (chr);

#elif defined(__HIGHC__)     /* outch() must not be optimised! */
  _inline (0x8A,0x55,0x08);  /* mov dl,[ebp+8] */
  _inline (0xB4,0x02);       /* mov ah,2       */
  _inline (0xCD,0x21);       /* int 21h        */

#elif defined(__GNUC__)
  __asm__ __volatile__
          ("movb %b0, %%dl\n\t"
           "movb $2, %%ah\n\t"
           "int $0x21\n\t"
           :
           : "r" (chr)
           : "%eax", "%edx" );

#else
  #error Tell me how to do this
#endif
}

void (*_outch)(char c) = outch;

/*---------------------------------------------------*/

#if defined(USE_DEBUG)
  #if defined(WATT32_DOS_DLL)   /* cannot use 'import_export.printf' here */
    int (MS_CDECL *_printf) (const char*, ...) = NULL;
  #else
    int (MS_CDECL *_printf) (const char*, ...) = printf;
  #endif
#else
  static int MS_CDECL EmptyPrint (const char *fmt, ...)
  {
    outsnl ("`(*_printf)()' called outside `USE_DEBUG'");
    ARGSUSED (fmt);
    return (0);
  }
  int (MS_CDECL *_printf) (const char*, ...) = EmptyPrint;
#endif

/*---------------------------------------------------*/

void outs (const char *s)
{
  while (s && *s)
  {
    if (*s == '\n')
       (*_outch) ('\r');
    (*_outch) (*s++);
  }
}

void outsnl (const char *s)
{
  outs (s);
  (*_outch) ('\r');
  (*_outch) ('\n');
}

void outsn (const char *s, int n)
{
  while (*s != '\0' && n-- >= 0)
  {
    if (*s == '\n')
       (*_outch) ('\r');
    (*_outch) (*s++);
  }
}

void outhex (char c)
{
  char hi = (c & 0xF0) >> 4;
  char lo = c & 15;

  if (hi > 9)
       (*_outch) ((char)(hi-10+'A'));
  else (*_outch) ((char)(hi+'0'));

  if (lo > 9)
       (*_outch) ((char)(lo-10+'A'));
  else (*_outch) ((char)(lo+'0'));
}

void outhexes (const char *s, int n)
{
  while (n-- > 0)
  {
    outhex (*s++);
    (*_outch) (' ');
  }
}

/**
 * Removes end-of-line termination from a string.
 * Removes "\n" (Unix), "\r" (MacOS) or "\r\n" (DOS/net-ascii)
 * terminations, but not single "\n\r" (who uses that?).
 */
char *rip (char *s)
{
  char *p;

  if ((p = strrchr(s,'\n')) != NULL) *p = '\0';
  if ((p = strrchr(s,'\r')) != NULL) *p = '\0';
  return (s);
}

/**
 * Convert hexstring "0x??" to hex. Just assumes "??"
 * are in the [0-9,a-fA-F] range. Don't call atox() unless
 * they are or before checking for a "0x" prefix.
 */
BYTE atox (const char *hex)
{
  unsigned hi = toupper (hex[2]);
  unsigned lo = toupper (hex[3]);

  hi -= isdigit (hi) ? '0' : 'A'-10;
  lo -= isdigit (lo) ? '0' : 'A'-10;
  return (BYTE) ((hi << 4) + lo);
}

/**
 * Replace 'ch1' to 'ch2' in string.
 */
char *strreplace (int ch1, int ch2, char *str)
{
  char *s = str;

  WATT_ASSERT (str != NULL);
  while (*s)
  {
    if (*s == ch1)
        *s = ch2;
    s++;
  }
  return (str);
}


/**
 * Similar to strncpy(), but always returns 'dst' with 0-termination.
 * \note Don't call this strlcpy() in case Watt-32 was compiled with
 *       djgpp 2.03, but linked with 2.04 (which already have strlcpy).
 *       And vice-versa.
 */
char *StrLcpy (char *dst, const char *src, size_t len)
{
  WATT_ASSERT (src != NULL);
  WATT_ASSERT (dst != NULL);
  WATT_ASSERT (len > 0);

  if (strlen(src) < len)
     return strcpy (dst, src);

  memcpy (dst, src, len);
  dst [len-1] = '\0';
  return (dst);
}

/**
 * Return pointer to first non-blank (space/tab) in a string.
 */
char *strltrim (const char *s)
{
  const char *p = s;

  WATT_ASSERT (s != NULL);

  while (p[0] && p[1] && isspace(*p))
       p++;
  return (char*)p;
}

/**
 * Trim trailing blanks (space/tab) from a string.
 */
char *strrtrim (char *s)
{
  size_t n;

  WATT_ASSERT (s != NULL);

  n = strlen (s);
  while (n)
  {
    if (!isspace(s[--n]))
       break;
    s[n] = '\0';
  }
  return (s);
}

/**
 * Copy a string, stripping leading and trailing blanks (space/tab).
 *
 * \note This function does not work exactly like strncpy(), in that it
 *       expects the destination buffer to be at last (n+1) chars long.
 */
size_t strntrimcpy (char *dst, const char *src, size_t n)
{
  size_t      len;
  const char *s;

  WATT_ASSERT (src != NULL);
  WATT_ASSERT (dst != NULL);

  len = 0;
  s   = src;

  if (n && s[0])
  {
    while (isspace(*s))
          ++s;
    len = strlen (s);
    if (len)
    {
      const char *e = &s[len-1];
      while (isspace(*e))
      {
        --e;
        --len;
      }
      if (len > n)
          len = n;
      strncpy (dst, s, len);
    }
  }
  if (len < n)
     dst[len] = '\0';
  return (len);
}

#ifdef NOT_USED
int isstring (const char *str, size_t len)
{
  if (strlen(str) > len-1)
     return (0);

  while (*str)
  {
    if (!isprint(*str++))
    {
      str--;
      if (!isspace(*str++))
         return (0);
    }
  }
  return (1);
}
#endif
