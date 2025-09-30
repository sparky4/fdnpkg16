/*!\file getopt.c
 * Parse command-line options.
 */

/* (emx+gcc) -- Copyright (c) 1990-1993 by Eberhard Mattes
 * Adapted for Waterloo TCP/IP by G. Vanem Nov-96
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(__DJGPP__)
/*
 * Must compile this with MingW so a WATT-32.dll built with
 * MingW can be used with MSVC.
 */
#include "wattcp.h"
#include "language.h"
#include "strings.h"
#include "getopt.h"

char *optarg          = NULL;
int   optopt          = 0;
int   optind          = 0;        /* Default: first call */
int   opterr          = 1;        /* Default: error messages enabled */
char *optswchar       = "-/";     /* '-' or '/' starts options */
enum _optmode optmode = GETOPT_UNIX;

static char *next_opt;        /* Next character in cluster of options */
static char *empty = "";      /* Empty string */

static BOOL done;
static char sw_char;

static char **options;        /* List of entries which are options */
static char **non_options;    /* List of entries which are not options */
static int    options_count;
static int    non_options_count;

#define PUT(dst) do {                                    \
                   if (optmode == GETOPT_ANY)            \
                      dst[dst##_count++] = argv[optind]; \
                  } while (0)

#undef ERROR

#if defined(USE_DEBUG)
  #define ERROR(str,fmt,ch)  printf (str), printf (fmt, ch), puts ("")
#else    /* avoid pulling in printf() */
  #define ERROR(str,fmt,ch)  outsnl (str)
#endif


int getopt (int argc, char *const *_argv, const char *opt_str)
{
  char  c;
  char *q;
  int   i, j;
  char **argv = (char **) _argv;

  if (optind == 0)
  {
    optind   = 1;
    done     = FALSE;
    next_opt = empty;
    if (optmode == GETOPT_ANY)
    {
      options     = (char**) malloc (argc * sizeof(char*));
      non_options = (char**) malloc (argc * sizeof(char*));
      if (!options || !non_options)
      {
        outsnl ("out of memory (getopt)");
        exit (255);
      }
      options_count     = 0;
      non_options_count = 0;
    }
  }
  if (done)
     return (EOF);

restart:
  optarg = NULL;
  if (*next_opt == 0)
  {
    if (optind >= argc)
    {
      if (optmode == GETOPT_ANY)
      {
        j = 1;
        for (i = 0; i < options_count; ++i)
            argv[j++] = options[i];
        for (i = 0; i < non_options_count; ++i)
            argv[j++] = non_options[i];
        optind = options_count+1;
        free (options);
        free (non_options);
      }
      done = TRUE;
      return (EOF);
    }
    else if (!strchr (optswchar, argv[optind][0]) || argv[optind][1] == 0)
    {
      if (optmode == GETOPT_UNIX)
      {
        done = TRUE;
        return (EOF);
      }
      PUT (non_options);
      optarg = argv[optind++];
      if (optmode == GETOPT_ANY)
         goto restart;
      /* optmode==GETOPT_KEEP */
      return (0);
    }
    else if (argv[optind][0] == argv[optind][1] && argv[optind][2] == 0)
    {
      if (optmode == GETOPT_ANY)
      {
        j = 1;
        for (i = 0; i < options_count; ++i)
            argv[j++] = options[i];
        argv[j++] = argv[optind];
        for (i = 0; i < non_options_count; ++i)
            argv[j++] = non_options[i];
        for (i = optind+1; i < argc; ++i)
            argv[j++] = argv[i];
        optind = options_count+2;
        free (options);
        free (non_options);
      }
      ++optind;
      done = TRUE;
      return (EOF);
    }
    else
    {
      PUT (options);
      sw_char  = argv[optind][0];
      next_opt = argv[optind]+1;
    }
  }
  c = *next_opt++;
  if (*next_opt == 0)  /* Move to next argument if end of argument reached */
     optind++;
  if (c == ':' || (q = strchr (opt_str, c)) == NULL)
  {
    if (opterr)
    {
      if (c < ' ' || c >= 127)
           ERROR ("Invalid option", "; character code=0x%.2x", c);
      else ERROR ("Invalid option", "/%c", c);
    }
    optopt = '?';
    return ('?');
  }
  if (q[1] == ':')
  {
    if (*next_opt != 0)         /* Argument given */
    {
      optarg = next_opt;
      next_opt = empty;
      ++optind;
    }
    else if (q[2] == ':')
      optarg = NULL;            /* Optional argument missing */
    else if (optind >= argc)
    {                           /* Required argument missing */
      if (opterr)
         ERROR ("No argument for option"," `/%c'", c);
      c = '?';
    }
    else
    {
      PUT (options);
      optarg = argv[optind++];
    }
  }
  optopt = c;
  return (c);
}
#endif  /* !__DJGPP__ && !__MINGW32__ */

