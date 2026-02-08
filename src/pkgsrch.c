/*
 * This file is part of FDNPKG16
 * Copyright (C) 2012-2016 Mateusz Viste
 * Copyright (C) 2025-2026 Victoria Crenshaw aka sparky4 && Katheryn Northwood
 */

#include <stdio.h>
#include <string.h>   /* strstr() */
#include <stdlib.h>   /* malloc(), free() */
#include <ctype.h>    /* toupper() */

#include "kprintf.h"
#include "pkgsrch.h"
#include "helpers.h"  /* fdnpkg16_strcasestr() */
#include "version.h"


void pkgsearch(struct pkgdb *pkgdb, char *searchterm, int verbosemode, char **repolist) {
  int matchflag, matchtimes = 0;
  struct pkgdb *curpkg;
  struct pkgrepo *currep;
  char *linebuf;
  puts("");
  linebuf = malloc(80);
  if (linebuf == NULL) {
    kitten_puts(5, 1, "Out of memory while processing package descriptions!");
    return;
  }
  for (curpkg = pkgdb->nextpkg; curpkg != NULL; curpkg = curpkg->nextpkg) {
    if (searchterm == NULL) {
      matchflag = 1;
    } else {
      matchflag = 0;
      if (fdnpkg16_strcasestr(curpkg->name, searchterm) != NULL) matchflag = 1; /* look into pkg name */
      if (fdnpkg16_strcasestr(curpkg->desc, searchterm) != NULL) matchflag = 1; /* look into pkg desc */
    }
    if (matchflag != 0) { /* display the package */
      if (snprintf(linebuf, 80, "%s - %s", curpkg->name, curpkg->desc) > 79) { /* truncated */
        linebuf[76] = '.';
        linebuf[77] = '.';
        linebuf[78] = '.';
        linebuf[79] = 0; /* finish the string - snprintf() is supposed to do it, but it appears the DJGPP version doesn't */
      }
      puts(linebuf);
      if (verbosemode != 0) {
        for (currep = curpkg->repolist; currep != NULL; currep = currep->nextrepo) {
          printf(" -> ver %s at %s [%08lX]\n", currep->version, repolist[currep->repo], currep->crc32zip);
        }
        puts("");
      }
      matchtimes++;
    }
  }
  free(linebuf);
  if (matchtimes == 0) kitten_puts(5, 0, "No package matched the search.");
}
