/*!\file getopt.h
 */
#ifndef _w32_GETOPT_H
#define _w32_GETOPT_H

#if defined(__DJGPP__)
  #include <unistd.h>

#else
  #define optarg    NAMESPACE (optarg)
  #define optind    NAMESPACE (optind)
  #define opterr    NAMESPACE (opterr)
  #define optopt    NAMESPACE (optopt)
  #define optswchar NAMESPACE (optswchar)
  #define optmode   NAMESPACE (optmode)
  #define getopt    NAMESPACE (getopt)

  W32_DATA char *optarg;     /* argument of current option                    */
  W32_DATA int   optind;     /* index of next argument; default=0: initialize */
  W32_DATA int   opterr;     /* 0=disable error messages; default=1: enable   */
  W32_DATA char *optswchar;  /* characters prefixing options; default="-"     */

  W32_DATA enum _optmode {
           GETOPT_UNIX,      /* options at start of argument list (default)   */
           GETOPT_ANY,       /* move non-options to the end                   */
           GETOPT_KEEP,      /* return options in order                       */
         } optmode;

  W32_FUNC int getopt (int argc, char *const *argv, const char *opt_str);

#endif
#endif
