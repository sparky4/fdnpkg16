#ifndef _w32_SYSDEP_H
#define _w32_SYSDEP_H

/* Dirty system-dependent hacks.
 */

#include <string.h>
#include <ctype.h>

#if defined(__MINGW32__)
  #define WIN32_LEAN_AND_MEAN

  #include <io.h>
  #include <getopt.h>
  #include <windows.h>

#elif defined(_MSC_VER)
  #define WIN32_LEAN_AND_MEAN

  #include <stdio.h>
  #include <io.h>
  #include <windows.h>
  #include "../src/getopt.c"

#elif defined(__DJGPP__)
  #include <dir.h>
  #include <io.h>
  #include <unistd.h>

#elif defined(__WATCOMC__) && !defined(__UNIX__) /* WATCOM DOS/WIN */
  #include <direct.h>
  #include <io.h>

#elif defined(__MINGW32__) || defined(__CYGWIN__) || defined(__WATCOMC__)
  #include <unistd.h>

#elif defined(__unix__) || defined(__linux__)
  #include <unistd.h>

  /* Cross compiling from Linux->DOS (assume gcc used)
   */
  static __inline char *my_strlwr (char *str)
  {
    char *c = str;
    for ( ; *c; c++)
       *c = tolower (*c);
    return (str);
  }
  static __inline char *my_strupr (char *str)
  {
    char *c = str;
    for ( ; *c; c++)
       *c = toupper (*c);
    return (str);
  }
  #define stricmp   strcasecmp
  #define strnicmp  strncasecmp
  #define strlwr    my_strlwr
  #define strupr    my_strupr

#else
  #error "Unsupported platform / cross-compile environment."
#endif

/*
 * mkdir() hackery.
 * djgpp does support the mode arg, not sure whether the value matters.
 */
#if defined(__DJGPP__)
  #define MKDIR(D) mkdir ((D), 0777)
  #define SLASH    '\\'

#elif defined(__WATCOMC__) && !defined(__UNIX__) /* WATCOM DOS/WIN */
  #define MKDIR(D)  mkdir ((D))
  #define SLASH     '\\'

#elif defined(__MINGW32__) || defined(__MSDOS__) /* MinGW / DJGPP */
  #define MKDIR(D)  mkdir ((D))
  #define SLASH     '\\'

#elif defined(_WIN32)                            /* MSVC, etc. */
  #define MKDIR(D) _mkdir ((D))
  #define SLASH    '\\'

#else /* assume unix: */
  #define MKDIR(D)  mkdir ((D),0755)
  #define SLASH     '/'
#endif

#endif  /* _w32_SYSDEP_H */
