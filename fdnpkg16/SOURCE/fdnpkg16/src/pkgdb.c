/*
 * Package database manipulation routines.
 * This file is part of the FDNPKG project.
 *
 * Copyright (C) 2012-2017 Mateusz Viste
 */


#include <stdio.h>  /* printf(), puts()... */
#include <stdlib.h> /* malloc(), free()... */
#include <string.h> /* strcmp() */
#include <strings.h>/* strcasecmp() */
#include <time.h>   /* time() */

#include "getdelim.h" /* a getdelim() implementation - there is none availble on DJGPP */
#include "kprintf.h"
#include "pkgdb.h"
#include "rtrim.h"
#include "version.h"


static int hex2crc(char *hexstr, unsigned long *result) {
  int x, tmpval;
  if ((result == NULL) || (hexstr == NULL)) return(-1);
  *result = 0;
  for (x = 0; x < 8; x++) {
    switch (hexstr[x]) {
      case '0':
        tmpval = 0;
        break;
      case '1':
        tmpval = 1;
        break;
      case '2':
        tmpval = 2;
        break;
      case '3':
        tmpval = 3;
        break;
      case '4':
        tmpval = 4;
        break;
      case '5':
        tmpval = 5;
        break;
      case '6':
        tmpval = 6;
        break;
      case '7':
        tmpval = 7;
        break;
      case '8':
        tmpval = 8;
        break;
      case '9':
        tmpval = 9;
        break;
      case 'A':
        tmpval = 10;
        break;
      case 'B':
        tmpval = 11;
        break;
      case 'C':
        tmpval = 12;
        break;
      case 'D':
        tmpval = 13;
        break;
      case 'E':
        tmpval = 14;
        break;
      case 'F':
        tmpval = 15;
        break;
      default:
        *result = 0;
        return(-1);
        break;
    }
    *result <<= 4;
    *result |= tmpval;
  }
  return(0);
}


/* Free the memory occupied by the linked list */
void freedb(struct pkgdb **db) {
  struct pkgdb *nextdb, *curdb;
  struct pkgrepo *nextrepo, *currepo;
  curdb = *db;
  while (curdb != NULL) {
    nextdb = curdb->nextpkg;
    /* free the package description */
    if (curdb->desc != NULL) free(curdb->desc);
    /* free all possible repositories */
    currepo = curdb->repolist;
    while (currepo != NULL) {
      nextrepo = currepo->nextrepo;
      free(currepo);
      currepo = nextrepo;
    }
    free(curdb);
    curdb = nextdb;
  }
}


/* Create the initial stucture for the db (first node) */
struct pkgdb *createdb(void) {
  struct pkgdb *newentry;
  newentry = malloc(sizeof(struct pkgdb));
  if (newentry == NULL) return(NULL);
  newentry->repolist = NULL;
  newentry->nextpkg = NULL;
  newentry->desc = NULL;
  return(newentry);
}


/* adds a new entry to the db - returns a pointer to the new entry on success, NULL otherwise */
static struct pkgdb *addpkg(struct pkgdb *db, char *pkgname, char *desc) {
  struct pkgdb *oldnextdb, *newentry;
  newentry = createdb();
  if (newentry == NULL) return(NULL);
  /* newentry allocation succeeded, let's allocated its children */
  strncpy(newentry->name, pkgname, 8);
  newentry->name[8] = 0; /* make sure to terminate the string, strncpy do not guarantee it */
  newentry->desc = strdup(desc);
  if ((newentry->name == NULL) || (newentry->desc == NULL)) {
    if (newentry->name != NULL) free(newentry->name);
    if (newentry->desc != NULL) free(newentry->desc);
    free(newentry);
    return(NULL);
  }
  if (strlen(desc) > 78) { /* if desc was longer than 78 bytes, terminate it nicely */
    newentry->desc[76] = '.';
    newentry->desc[77] = '.';
    newentry->desc[78] = '.';
    newentry->desc[79] = 0;
  }
  oldnextdb = db->nextpkg;
  db->nextpkg = newentry;
  newentry->nextpkg = oldnextdb;
  return(newentry);
}


/* adds a new repo to entry *db - returns the new entry address on success, NULL otherwise */
static struct pkgrepo *addrepo(struct pkgdb *pkg, unsigned char repo, const char *version, unsigned long crc32zip, unsigned long crc32zib) {
  struct pkgrepo *oldnextrepo, *newrepo;
  newrepo = calloc(sizeof(struct pkgrepo), 1);
  if (newrepo == NULL) return(NULL);
  newrepo->nextrepo = pkg->repolist;
  strncpy(newrepo->version, version, 15);
  newrepo->version[15] = 0; /* make sure the string is NULL terminated - strncpy do not guarantee that */
  newrepo->repo = repo;
  newrepo->crc32zip = crc32zip;
  newrepo->crc32zib = crc32zib;
  oldnextrepo = pkg->repolist;
  pkg->repolist = newrepo;
  newrepo->nextrepo = oldnextrepo;
  return(newrepo);
}


/* Searches after a pkg named 'pkgname' in the db. Returns a pointer to the package if found, or NULL otherwise.
   If no package found, then lastmatch is set to the last alphabetically inferior package */
struct pkgdb *findpkg(struct pkgdb *db, char *pkgname, struct pkgdb **lastmatch) {
  struct pkgdb *curpkg;
  int cmpres;
  *lastmatch = db;
  for (curpkg = db->nextpkg; curpkg != NULL; curpkg = curpkg->nextpkg) {
    cmpres = strcasecmp(curpkg->name, pkgname);
    if (cmpres < 0) {    /* if alphabetically inferior, remember lastmatch */
      *lastmatch = curpkg;
    } else if (cmpres == 0) { /* if equal, then we found what we came for! */
      return(curpkg);
    } else { /* if superior, then there's no point in loosing more time here (assuming the list is sorted) */
      return(NULL);
    }
  }
  return(NULL);
}


static void getpkgmetadata(char *lineptr, char **pkgmeta) {
  int x = 0, y = 1, lastnonemptychar = 0;
  pkgmeta[0] = lineptr;
  pkgmeta[1] = lineptr;
  pkgmeta[2] = lineptr;
  pkgmeta[3] = lineptr;
  pkgmeta[4] = lineptr;
  while (lineptr[x] != 0) {
    /* check out what character we have, and remember if it's whitespace or not */
    switch (lineptr[x]) {
      case '\t': /* delimiters */
      case '\r':
      case '\n':
        lineptr[lastnonemptychar + 1] = 0;
        if (y <= 4) pkgmeta[y] = &lineptr[x + 1];
        lastnonemptychar = x + 1;
        y++;
        if (y > 5) return;
        break;
      case ' ': /* whitespace... */
        break;
      default: /* remember last non-whitespace char */
        lastnonemptychar = x;
    }
    x++;
  }
}


int loaddb_fromcache(struct pkgdb *db, char *datafile, unsigned long crc32val, long maxcachetime) {
  time_t curtime;
  unsigned long uint32val, crc32zip, crc32zib;
  char *pkgname, *pkgdesc;
  struct pkgdb *pkg;
  int tmpchar, cursor;
  unsigned char repo;
  FILE *fd;
  fd = fopen(datafile, "rb");
  if (fd == NULL) return(-1);
  if (fgetc(fd) != 2) {
    fclose(fd);
    return(-1);  /* read and verify the first byte (version identifier) */
  }
  uint32val = fgetc(fd);
  uint32val <<= 8;
  uint32val |= fgetc(fd);
  uint32val <<= 8;
  uint32val |= fgetc(fd);
  uint32val <<= 8;
  uint32val |= fgetc(fd);
  if (uint32val != crc32val) {
    fclose(fd);
    return(-1);
  }
  curtime = time(NULL);
  uint32val = fgetc(fd);
  uint32val <<= 8;
  uint32val |= fgetc(fd);
  uint32val <<= 8;
  uint32val |= fgetc(fd);
  uint32val <<= 8;
  uint32val |= fgetc(fd);
  if (curtime < uint32val) { /* cache is from the future, we don't like this */
    fclose(fd);
    return(-1);
  }
  if (curtime > uint32val + maxcachetime) { /* cache is too old */
    fclose(fd);
    return(-1);
  }
  if (fgetc(fd) != 0x0A) { /* we expect a LF here.. */
    fclose(fd);
    return(-1);
  }
  /* allocated some memory for doing stuff */
  pkgname = malloc(64);
  pkgdesc = malloc(1024);
  if ((pkgname == NULL) || (pkgdesc == NULL)) {
    free(pkgname);
    free(pkgdesc);
    fclose(fd);
    return(-2);
  }
  pkg = db;
  for (;;) {
    /* read the first byte, to be able to determine whether EOF is reached */
    tmpchar = fgetc(fd);
    if (tmpchar < 0) break; /* EOF -> get out! */
    /* Get pkgname */
    for (cursor = 0;;) {
      if (cursor > 0) tmpchar = fgetc(fd);
      if (tmpchar == '\t') break; /* end of record */
      if ((tmpchar < 32) || (cursor > 62)) { /* invalid char or unexpected enf of file or pkgname too long == corruption */
        free(pkgname);
        free(pkgdesc);
        fclose(fd);
        return(-1);
      }
      pkgname[cursor++] = tmpchar;
    }
    pkgname[cursor] = 0; /* terminate the pkgname string */
    /* Get pkgdesc */
    for (cursor = 0;;) {
      tmpchar = fgetc(fd);
      if (tmpchar == '\t') break; /* end of record */
      if ((tmpchar < 32) || (cursor > 1022)) { /* invalid char or unexpected enf of file or pkgdesc too long == corruption */
        free(pkgname);
        free(pkgdesc);
        fclose(fd);
        return(-1);
      }
      pkgdesc[cursor++] = tmpchar;
    }
    pkgdesc[cursor] = 0; /* terminate the pkgdesc string */
    /* add the package entry into the db - SORTED (important!) */
    /* look if package already exists in db... we can't skip this even if the package won't ever be present in cache twice, because we use this also to sort packages (and find them faster later)
     Note, that I do NOT need to sort entries here, because I know they already come sorted, since I grab them
     from my own cache. */
    pkg = addpkg(pkg, pkgname, pkgdesc);
    if (pkg == NULL) { /* Out of memory! */
      kitten_puts(6, 2, "Error: Out of memory while loading package database!");
      free(pkgname);
      free(pkgdesc);
      fclose(fd);
      return(-1);
    }
    /* now retrieve repositories */
    do {
      /* Get repo */
      tmpchar = fgetc(fd);
      if (tmpchar < 0) { /* unexpected end == corruption */
        free(pkgname);
        free(pkgdesc);
        fclose(fd);
        return(-1);
      }
      repo = tmpchar;
      /* Get the CRC32 value of the package (ZIP) */
      crc32zip = fgetc(fd);
      crc32zip <<= 8;
      crc32zip |= fgetc(fd);
      crc32zip <<= 8;
      crc32zip |= fgetc(fd);
      crc32zip <<= 8;
      crc32zip |= fgetc(fd);
      /* Get the CRC32 value of the package (ZIB) */
      crc32zib = fgetc(fd);
      crc32zib <<= 8;
      crc32zib |= fgetc(fd);
      crc32zib <<= 8;
      crc32zib |= fgetc(fd);
      crc32zib <<= 8;
      crc32zib |= fgetc(fd);
      /* Get version */
      for (cursor = 0;;) {
        tmpchar = fgetc(fd);
        if (tmpchar == '\t') break; /* end of record */
        if (tmpchar == '\n') break; /* end of record (and this is the last one) */
        if ((tmpchar < 32) || (cursor > 62)) { /* invalid char or unexpected enf of file or pkgdesc too long == corruption */
          free(pkgname);
          free(pkgdesc);
          fclose(fd);
          return(-1);
        }
        pkgname[cursor++] = tmpchar;
      }
      pkgname[cursor] = 0; /* terminate the pkgname string */
      /* add the repo to the list of repos for the current package */
      if (addrepo(pkg, repo, pkgname, crc32zip, crc32zib) == NULL) { /* Out of memory! */
        kitten_puts(6, 2, "Error: Out of memory while loading package database!");
        free(pkgname);
        free(pkgdesc);
        fclose(fd);
        return(-1);
      }
    } while (tmpchar == '\t');
  }
  free(pkgname);
  free(pkgdesc);
  fclose(fd);
  return(0);
}


#define MAXDBMSGSIZE 1024 /* max lenght of a msg coming from the repository */

/* Load the db from file - returns 0 on success, non-zero on failure. */
int loaddb(struct pkgdb *db, char *datafile, unsigned char repo, char **dbmsg) {
  FILE *fd;
  char *lineptr;
  char *pkgmeta[8];
  struct pkgdb *pkg, *lastmatch;
  int result = 0;
  size_t getdelimcount = 0;
  int getdelimlen;
  unsigned long crc32zip = 0, crc32zib = 0;

  fd = fopen(datafile, "r");
  if (fd == NULL) {
    kitten_printf(6, 3, "Error: Could not open the datafile at '%s'", datafile);
    puts("");
    return(-1);
  }

  *dbmsg = NULL;

  /* Check whether the index file has a valid header */
  lineptr = NULL;
  getdelimlen = getdelim(&lineptr, &getdelimcount, '\n', fd);
  if (getdelimlen < 0) {
    if (lineptr != NULL) free(lineptr);
    kitten_puts(6, 0, "Error: invalid index file (bad header)! Repository skipped.");
    result = -6;
  } else {
    getpkgmetadata(lineptr, pkgmeta); /* pkgmeta[0] = "FD-REPOv1" */
    if (strcmp(pkgmeta[0], "FD-REPOv1") != 0) {
      free(lineptr);
      kitten_puts(6, 0, "Error: invalid index file (bad header)! Repository skipped.");
      result = -7;
    } else {
      /* start reading the index content */
      for (;;) {
        /* read line from file */
        lineptr = NULL;
        getdelimlen = getdelim(&lineptr, &getdelimcount, '\n', fd);
        if (getdelimlen < 0) {
          if (lineptr != NULL) free(lineptr);
          break;
        }
        /* printf("Got line: %s\n", lineptr); */
        if ((lineptr[0] == 0) || (lineptr[0] == '\r') || (lineptr[0] == '\n')) continue; /* skip empty lines */
        /* if it's a message, remember it and skip to next line */
        if (lineptr[0] == '.') {
          if (*dbmsg == NULL) {
            *dbmsg = malloc(MAXDBMSGSIZE);
            if (*dbmsg == NULL) continue; /* if malloc() failed, then silently continue... */
            *dbmsg[0] = 0;
          }
          rtrim(lineptr); /* remove lf, cr and cr/lf trailers */
          strncat(*dbmsg, lineptr+1, MAXDBMSGSIZE - 1);
          strncat(*dbmsg, "\r\n", MAXDBMSGSIZE - 1);
          continue;
        }
        /* get pkgname and version */ /* PkgName <tab> version <tab> description */
        getpkgmetadata(lineptr, pkgmeta); /* pkgmeta[0] = PkgName ; pkgmeta[1] = version ; pkgmeta[2] = description ; pkgmeta[3] = CRC32 ; pkgmeta[4] = CRC32 zib */
        if (hex2crc(pkgmeta[3], &crc32zip) != 0) {
          /* oops, no a valid CRC32! This indicates a corrupted download */
          if (lineptr != NULL) free(lineptr);
          kitten_puts(6, 1, "Error: invalid index file! Repository skipped.");
          result = -7;
          break;
        }
        if (hex2crc(pkgmeta[4], &crc32zib) != 0) crc32zib = 0;

        /* look if package already exists in db... */
        pkg = findpkg(db, pkgmeta[0], &lastmatch);
        /* if not -> add the package to the db first */
        if (pkg == NULL) {
          pkg = addpkg(lastmatch, pkgmeta[0], pkgmeta[2]);
          if (pkg == NULL) {
            kitten_puts(6, 2, "Error: Out of memory while loading package database!");
            if (lineptr != NULL) free(lineptr);
            result = -1;
            break;
          }
        }
        /* now add the repository to the package */
        if (addrepo(pkg, repo, pkgmeta[1], crc32zip, crc32zib) == NULL) { /* ooops, something went wrong */
          kitten_puts(6, 2, "Out of memory while loading package database!");
          result = -1;
          if (lineptr != NULL) free(lineptr);
          break;
        }
        if (lineptr != NULL) free(lineptr); /* free the memory occupied by the line */
      }
    }
  }
  fclose(fd);
  return(result);
}


/* dumps the entire content of the db into a cache file */
void dumpdb(struct pkgdb *db, char *datafile, unsigned long crc32val) {
  FILE *fd;
  time_t curtime;
  struct pkgdb *curnode;
  struct pkgrepo *currepo;
  fd = fopen(datafile, "wb");
  if (fd == NULL) {
    kitten_printf(6, 4, "Warning: Could not open db cache file at %s!", datafile);
    puts("");
    return;
  }
  kitten_puts(2, 20, "Package database is being created. Please wait warmly."); // sparky4: warmly means here patiently
  curtime = time(NULL);
  /* write the file format version */
  fprintf(fd, "%c", 2);
  /* write the CRC32 of the configfile */
  fprintf(fd, "%c%c%c%c", (int)(crc32val >> 24) & 0xFF, (int)(crc32val >> 16) & 0xFF, (int)(crc32val >> 8) & 0xFF, (int)(crc32val & 0xFF));
  /* write the current time, followed by a LF character */
  fprintf(fd, "%c%c%c%c%c", (int)(curtime >> 24) & 0xFF, (int)(curtime >> 16) & 0xFF, (int)(curtime >> 8) & 0xFF, (int)(curtime & 0xFF), 0x0A);
  /* start dumping records (PKGNAME, PKGDESC, REPO1, VER1, REPO2, VER2, ..., LF) */
  for (curnode = db->nextpkg; curnode != NULL; curnode = curnode->nextpkg) {
    fprintf(fd, "%s\t%s", curnode->name, curnode->desc);
    for (currepo = curnode->repolist; currepo != NULL; currepo = currepo->nextrepo) {
      fprintf(fd, "\t%c%c%c%c%c%c%c%c%c%s", currepo->repo, (int)(currepo->crc32zip >> 24) & 0xFF, (int)(currepo->crc32zip >> 16) & 0xFF, (int)(currepo->crc32zip >> 8) & 0xFF, (int)currepo->crc32zip & 0xFF, (int)(currepo->crc32zib >> 24) & 0xFF, (int)(currepo->crc32zib >> 16) & 0xFF, (int)(currepo->crc32zib >> 8) & 0xFF, (int)currepo->crc32zib & 0xFF, currepo->version);
    }
    fprintf(fd, "%c", 0x0A);
  }
  fclose(fd);
}
