/*
 * FDNPKG16 - FreeDOS Network Package manager
 * Copyright (C) 2012-2017 Mateusz Viste
 * Copyright (C) 2025 Victoria Crenshaw aka sparky4
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */


#include <ctype.h>    /* tolower() */
#include <stdio.h>    /* printf() */
#include <stdlib.h>   /* free() */
#include <string.h>   /* strcasecmp() */
#include <strings.h>  /* strcasecmp() */
#include <unistd.h>   /* unlink() */
#include <conio.h>    /* getch() */
#include <malloc.h>   /* _heapmin() */

#include "fdnpkg16.h" /* this file */
#include "helpers.h"  /* various helper functions */
#ifndef USE_EXTERNAL_MTCP
#include "http.h"     /* http_get() */
#endif
#include "kitten.h"   /* required for kitten subsystem */
#include "kprintf.h"  /* kitten_printf() */
#include "libgz.h"    /* ungz() */
#include "libunzip.h" /* zip_freelist()... */
#include "loadconf.h" /* loadconf() */
#ifndef USE_EXTERNAL_MTCP
#include "net.h"      /* net_init() */
#endif
#include "pkgdb.h"    /* createdb(), findpkg()... */
#include "pkgsrch.h"  /* pkgsrch() */
#include "pkginst.h"  /* pkginstall() */
#include "pkgrem.h"   /* pkgrem() */
#include "readenv.h"  /* readenv() */
#include "showinst.h" /* showinstalledpkgs() */
#include "version.h"
#include "hc.h"

/* #define DEBUG */ /* uncomment this to enable debug mode */

//unsigned _stklen = /*512*/24 * 1024; /* I need 512K of stack space */ //not doable in 16 bit lets give it 24k

// sparky4: just some program naming her for fdnpkg16 and fdnpkg86
#ifndef USE_EXTERNAL_MTCP
#define EXECNAME "16"
extern char *wattcpVersion(); /* provided by wattcp to poll its version */
#else
#define EXECNAME "86"
#endif

static void printhelp(void) {
  puts("FDNPKG" EXECNAME " v" PVER " (C) " PDATE " Mateusz Viste && Victoria Crenshaw");
  kitten_puts(1, 0, "This is a network package manager for FreeDOS.");
  puts("");
  kitten_printf(1,1,"Usage: FDNPKG%s action [parameters]", EXECNAME);
  puts("");
  puts("");
  kitten_puts(1, 2,  "Where action is one of the following:");
  printf(" search [str]      "); kitten_puts(1, 3,  "- search net repositories for package containing 'str'");
  printf(" vsearch [str]     "); kitten_puts(1, 4,  "- same as 'search', but prints also source repositories");
  printf(" install pkg       "); kitten_puts(1, 5,  "- install the package 'pkg' (or local zip file)");
  printf(" install-nosrc pkg "); kitten_puts(1, 10, "- install the package 'pkg' (or local zip file) w/o sources");
  printf(" install-wsrc pkg  "); kitten_puts(1, 11, "- install the package 'pkg' (or local zip file) with sources");
  printf(" reinstall pkg     "); kitten_puts(1, 20, "- reinstall the package 'pkg' (or local zip file)");
  printf(" remove pkg        "); kitten_puts(1, 6,  "- remove the package 'pkg'");
  printf(" listlocal [str]   "); kitten_puts(1, 16, "- list all local (installed) packages containing 'str'");
  printf(" listfiles pkg     "); kitten_puts(1, 18, "- list files owned by the package 'pkg'");
  printf(" checkupdates      "); kitten_puts(1, 13, "- check for available updates of packages and display them");
  printf(" update [pkg]      "); kitten_puts(1, 15, "- update 'pkg' to last version (or all packages if no arg)");
  printf(" dumpcfg           "); kitten_puts(1, 7,  "- print out the configuration loaded from the cfg file");
  printf(" clearcache        "); kitten_printf(1,19,"- clear FDNPKG%s's local cache", EXECNAME); puts("");
  printf(" license           "); kitten_puts(1, 8,  "- print out the license of this program");
//  puts("");
#ifdef DEBUG
#if defined(__WATCOMC__)
#if (__WATCOMC__ >= 1200)
  printf ("FDNPKG%s is Compiled with OpenWatcom %d.%d\n", EXECNAME,
                  (__WATCOMC__/100) - 11, (__WATCOMC__ % 100) / 10);
#else
  printf ("FDNPKG%s is Compiled with Watcom C %d.%d\n", EXECNAME, __WATCOMC__/100, __WATCOMC__ % 100);
#endif /* #if (__WATCOMC__ >= 1200) */
#endif /* #if defined(__WATCOMC__) */
  puts("");
#endif /* #ifdef DEBUG */
#ifndef USE_EXTERNAL_MTCP
  kitten_printf(1, 9, "FDNPKG%s is linked against the Watt-32 version below:", EXECNAME);
  puts("");
  puts(wattcpVersion());
#else
  kitten_printf(1, 21, "FDNPKG%s is using mTCP", EXECNAME);
  puts("");
  puts("");
#endif
}
static void printhelpshort(void) {
  puts("FDNPKG" EXECNAME " v" PVER " (C) " PDATE " Mateusz Viste && Victoria Crenshaw");
  kitten_puts(1, 0, "This is a network package manager for FreeDOS.");
  puts("");
  kitten_printf(1,1,"Usage: FDNPKG%s action [parameters]", EXECNAME);
  puts("");
  puts("");
  kitten_puts(1, 2,  "Where action is one of the following:");
  printf(" se [str]          "); kitten_puts(1, 3,  "- search net repositories for package containing 'str'");
  printf(" vs [str]          "); kitten_puts(1, 4,  "- same as 'search', but prints also source repositories");
  printf(" in pkg            "); kitten_puts(1, 5,  "- install the package 'pkg' (or local zip file)");
  printf(" in-nosrc pkg      "); kitten_puts(1, 10, "- install the package 'pkg' (or local zip file) w/o sources");
  printf(" in-wsrc pkg       "); kitten_puts(1, 11, "- install the package 'pkg' (or local zip file) with sources");
  printf(" ri pkg            "); kitten_puts(1, 20, "- reinstall the package 'pkg' (or local zip file)");
  printf(" rm pkg            "); kitten_puts(1, 6,  "- remove the package 'pkg'");
  printf(" ll [str]          "); kitten_puts(1, 16, "- list all local (installed) packages containing 'str'");
  printf(" lf pkg            "); kitten_puts(1, 18, "- list files owned by the package 'pkg'");
  printf(" cu                "); kitten_puts(1, 13, "- check for available updates of packages and display them");
  printf(" up [pkg]          "); kitten_puts(1, 15, "- update 'pkg' to last version (or all packages if no arg)");
  printf(" dc                "); kitten_puts(1, 7,  "- print out the configuration loaded from the cfg file");
  printf(" cc                "); kitten_printf(1,19,"- clear FDNPKG%s's local cache", EXECNAME); puts("");
  printf(" li                "); kitten_puts(1, 8,  "- print out the license of this program");
//  puts("");
#ifdef DEBUG
#if defined(__WATCOMC__)
#if (__WATCOMC__ >= 1200)
  printf ("FDNPKG%s is Compiled with OpenWatcom %d.%d\n", EXECNAME,
                  (__WATCOMC__/100) - 11, (__WATCOMC__ % 100) / 10);
#else
  printf ("FDNPKG%s is Compiled with Watcom C %d.%d\n", EXECNAME, __WATCOMC__/100, __WATCOMC__ % 100);
#endif /* #if (__WATCOMC__ >= 1200) */
#endif /* #if defined(__WATCOMC__) */
  puts("");
#endif /* #ifdef DEBUG */
#ifndef USE_EXTERNAL_MTCP
  kitten_printf(1, 9, "FDNPKG%s is linked against the Watt-32 version below:", EXECNAME);
  puts("");
  puts(wattcpVersion());
#else
  kitten_printf(1, 21, "FDNPKG%s is using mTCP", EXECNAME);
  puts("");
  puts("");
#endif
}


static void printlic(void) {
  puts("FDNPKG" EXECNAME " v" PVER " - FreeDOS Network Package manager\r\n"
       "Copyright (C) " PDATE " Mateusz Viste && Victoria Crenshaw\r\n");

  puts("Permission is hereby granted, free of charge, to any person obtaining a copy\r\n"
       "of this software and associated documentation files (the \"Software\"), to deal\r\n"
       "in the Software without restriction, including without limitation the rights\r\n"
       "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\r\n"
       "copies of the Software, and to permit persons to whom the Software is\r\n"
       "furnished to do so, subject to the following conditions:\r\n");

  puts("The above copyright notice and this permission notice shall be included in\r\n"
       "all copies or substantial portions of the Software.\r\n");

  puts("THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\r\n"
       "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\r\n"
       "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\r\n"
       "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\r\n"
       "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING\r\n"
       "FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS\r\n"
       "IN THE SOFTWARE.");
  puts("\nIf you want to contribute, let me know! https://discord.com/invite/qBH9W7fXHJ\n"
       "or join my irc and ping me irc://4ch.mooo.com/#fdnpkg16");
}


enum actions {
  ACTION_HELP,
  ACTION_SEARCH,
  ACTION_INSTALL,
  ACTION_INSTALL_LOCALFILE,
  ACTION_UPGRADE,
  ACTION_REMOVE,
  ACTION_DUMPCFG,
  ACTION_LICENSE,
  ACTION_LISTLOCAL,
  ACTION_LISTFILES,
  ACTION_CHECKUPDATES,
  ACTION_UPDATE,
  ACTION_CLEARCACHE,
  ACTION_REINSTALL,
  ACTION_REINSTALL_LOCALFILE
};


/* try to create a temp file in the 'dirname' directory and write garbage to
 * it. returns 0 on success, non-zero on failure. */
static int trycreatefileindir(char *dirname) {
  FILE *fd;
  char filename[512];
  int writeres;
  if (dirname == NULL) return(-1);
  sprintf(filename, "%s\\fdnpkg16.tmp", dirname);
  fd = fopen(filename, "wb");
  if (fd == NULL) return(-1);
  writeres = fputs("FDNPKG16", fd);
  fclose(fd);
  if (writeres < 0) return(-1);
  return(0);
}


#ifndef DEBUG
#define QUIT(x) \
  kittenclose(); \
  return(x);
#else
#define QUIT(x) \
  kittenclose(); \
  return(x);
#endif


int main(int argc, char **argv) {
  long htgetres;
  char *tempdir;
  char cfgfile[256];
  char *dosdir;
  #define MAXREPS 64
  char *repolist[MAXREPS];
  int repolistcount;
  int verbosemode = 0;
  int flags = 0;
  long maxcachetime = 7200;
  char *proxy = NULL;
  char downloadingstring[64];
  int proxyport = 8080;
  int i,x,y;
  char *mapdrv = "";
  unsigned long cfgfilecrc;
  struct pkgdb *pkgdb = NULL;
  struct customdirs *dirlist = NULL;
  enum actions action = ACTION_HELP;
  FILE *zipfilefd;
  struct ziplist *zipfileidx;
  int netinitres;
#ifdef USE_EXTERNAL_MTCP
  char command[512];
  FILE *batch_file;
  char commandforbatch[512];
#endif
  int arglen;
  int argci;
  static char pkg[12];
  static char argone[16];

  strcpy(pkg, "");

  #ifdef DEBUG
  puts("DEBUG BUILD " __DATE__ " " __TIME__);
  #endif

  /* First init KITTEN */
  kittenopen("FDNPKG16");

  /* fetch the 'downloading...' kitten string, we will need to pass it to http_get later, eventually */
  sprintf(downloadingstring, "%s", kittengets(11, 0, "Downloading %s... %ld bytes"));

  /* read all required environment variables */
  if (readenv(&dosdir, &tempdir, cfgfile, sizeof(cfgfile), argv) != 0) {
    QUIT(0);
  }

  /* check the available memory and display a warning if too low */
  /* 128k */
#ifdef DEBUG
  printf("farcoreleft() == %ld\n", farcoreleft());
  //printf("ncoreleft() == %ld\n", ncoreleft());
  //printf("farcoreleft() == %lu    \n", farcoreleft());
  printf("coreleft() == %u\n", coreleft());
  getch();
#endif
  if (farcoreleft() < ((long)(131072))) {
    kitten_printf(2, 17, "WARNING: Virtual memory too low. FDNPKG%s might behave unreliably.", EXECNAME); puts("");
  } // sparky4: seems to work now with the new farcoreleft function from vfed!

  /* Load the list of package repositories */
  repolistcount = loadconf(cfgfile, repolist, MAXREPS, &cfgfilecrc, &maxcachetime, &dirlist, &flags, &proxy, &proxyport, &mapdrv);
  if (repolistcount < 0) return(5);

  // sparky4: check for /
  if (argc > 1) { // Ensure argv[1] exists
    if (argv[1][0] == '/') { // Check if the first character is '/'
      // Option 1: Shift the pointer to effectively remove the first character
      // This modifies what argone points to, but not the underlying string data
      strcpy(argone, argv[1]++);
    } else {
      strcpy(argone, argv[1]);
    }
  }

  argci = argc;

  // sparky4: get the for loop condition numbe ready! :D
  if (argc > 2) {
    argci = argc - 2;
  }
  /* sparky4: start of that huge for loop. This loop manages the packages in the argument list! :D */
  for (i = 0; i < argci; i++) {

  /* parse cli parameters */
  if (argc > 1) { /* fdnpkg action [param] */
    if ((argc > 2) && (argv[2] != NULL)) {
      strcpy(pkg, argv[i+2]);
    }
    if ((strcasecmp(argone, "search") && strcasecmp(argone, "se")) == 0) {
      if (argc < 3) {
        kitten_printf(2, 4, "Invalid number of arguments. Run FDNPKG%s without any parameter for help.", EXECNAME); puts("");
        QUIT(0)
      } else {
        action = ACTION_SEARCH;
      }
    } else if ((strcasecmp(argone, "vsearch") && strcasecmp(argone, "vs")) == 0) {
      if (argc < 3) {
        kitten_printf(2, 4, "Invalid number of arguments. Run FDNPKG%s without any parameter for help.", EXECNAME); puts("");
        QUIT(0)
      } else {
        action = ACTION_SEARCH;
        verbosemode = 1;
      }
    } else if (((strcasecmp(argone, "install") && strcasecmp(argone, "in")) == 0) || ((strcasecmp(argone, "install-nosrc") && strcasecmp(argone, "in-nosrc")) == 0) || ((strcasecmp(argone, "install-wsrc") && strcasecmp(argone, "in-wsrc")) == 0)) {
      if ((strcasecmp(argone, "install-nosrc") && strcasecmp(argone, "in-nosrc")) == 0) flags |= PKGINST_NOSOURCE;
      if ((strcasecmp(argone, "install-wsrc") && strcasecmp(argone, "in-wsrc")) == 0) flags &= ~(PKGINST_NOSOURCE);
      if (argc < 3) {
        kitten_printf(2, 4, "Invalid number of arguments. Run FDNPKG%s without any parameter for help.", EXECNAME); puts("");
        QUIT(0)
      } else {
        arglen = strlen(pkg);
        action = ACTION_INSTALL;
        if (arglen > 4) {
          if ((pkg[arglen - 4] == '.') && (tolower(pkg[arglen - 3]) == 'z') && (tolower(pkg[arglen - 2]) == 'i')) { /* if argument ends with '.zi?' (zip/zib), then it's a local package file */
            action = ACTION_INSTALL_LOCALFILE;
          }
        }
      }
    } else if ((strcasecmp(argone, "remove") && strcasecmp(argone, "rm")) == 0) {
      if (argc < 3) {
        kitten_printf(2, 4, "Invalid number of arguments. Run FDNPKG%s without any parameter for help.", EXECNAME); puts("");
      } else {
        pkgrem(pkg, dosdir, mapdrv);
      }
      QUIT(0);
    } else if ((strcasecmp(argone, "update") && strcasecmp(argone, "up")) == 0) {
      if (argc >= 3) {
        action = ACTION_UPDATE;
      } else if (argc == 2) {
        argci--; // sparky4: bug fix to prevent looping twice for these functions
        action = ACTION_UPGRADE;
      } else {
        kitten_printf(2, 4, "Invalid number of arguments. Run FDNPKG%s without any parameter for help.", EXECNAME); puts("");
        QUIT(0);
      }
    } else if (((strcasecmp(argone, "listlocal") && strcasecmp(argone, "ll")) == 0) || ((strcasecmp(argone, "showinstalled") && strcasecmp(argone, "si")) == 0)) { /* 'showinstalled' is the old name for 'listlocal' - retained for backward compatibility, but to be removed in some futur */
      if (argc < 3) {
        kitten_printf(2, 4, "Invalid number of arguments. Run FDNPKG%s without any parameter for help.", EXECNAME); puts("");
        QUIT(0);
      } else {
        action = ACTION_LISTLOCAL;
      }
    } else if ((strcasecmp(argone, "listfiles") && strcasecmp(argone, "lf")) == 0) {
      if (argc < 3) {
        kitten_printf(2, 4, "Invalid number of arguments. Run FDNPKG%s without any parameter for help.", EXECNAME); puts("");
        QUIT(0);
      } else {
        action = ACTION_LISTFILES;
      }
    } else if ((strcasecmp(argone, "dumpcfg") && strcasecmp(argone, "dc")) == 0) {
      if (argc != 2) {
        kitten_printf(2, 4, "Invalid number of arguments. Run FDNPKG%s without any parameter for help.", EXECNAME); puts("");
        QUIT(0)
      } else {
        argci--; // sparky4: bug fix to prevent looping twice for these functions
        action = ACTION_DUMPCFG;
      }
    } else if ((strcasecmp(argone, "license") && strcasecmp(argone, "li")) == 0) {
      if (argc != 2) {
        kitten_printf(2, 4, "Invalid number of arguments. Run FDNPKG%s without any parameter for help.", EXECNAME); puts("");
      } else {
        printlic();
      }
      QUIT(0)
    } else if ((strcasecmp(argone, "checkupdates") && strcasecmp(argone, "cu")) == 0) {
      if (argc != 2) {
        kitten_printf(2, 4, "Invalid number of arguments. Run FDNPKG%s without any parameter for help.", EXECNAME); puts("");
        QUIT(0)
      } else {
        argci--; // sparky4: bug fix to prevent looping twice for these functions
        action = ACTION_CHECKUPDATES;
      }
    } else if ((strcasecmp(argone, "clearcache") && strcasecmp(argone, "cc")) == 0) {
      if (argc != 2) {
        kitten_printf(2, 4, "Invalid number of arguments. Run FDNPKG%s without any parameter for help.", EXECNAME); puts("");
        QUIT(0)
      } else {
        action = ACTION_CLEARCACHE;
      }
    } else if ((strcasecmp(argone, "reinstall") && strcasecmp(argone, "ri")) == 0) {
      if (argc < 3) {
        kitten_printf(2, 4, "Invalid number of arguments. Run FDNPKG%s without any parameter for help.", EXECNAME); puts("");
        QUIT(0)
      } else {
        arglen = strlen(pkg);
        action = ACTION_REINSTALL;
        if (arglen > 4) {
          if ((pkg[arglen - 4] == '.') && (tolower(pkg[arglen - 3]) == 'z') && (tolower(pkg[arglen - 2]) == 'i')) { /* if argument ends with '.zi?' (zip/zib), then it's a local package file */
            action = ACTION_REINSTALL_LOCALFILE;
          }
        }
      }
      // sparky4: <3
    } else if ((strcasecmp(argone, "bibabo")) == 0) {
      printf("ビバボ！ｗ");
      QUIT(0)
    } else if ((strcasecmp(argone, "poi")) == 0) {
      printf("ぽい！ｗ");
      QUIT(0)
    }
  }

  /* Dual help! this runs both helps! :D */
  if (action == ACTION_HELP) {
    printhelp();
    printf("Press any key but Q to continue...\n");
    while ((y = getch()) == 'q') { QUIT(0) }
    printhelpshort();
    puts("");
    QUIT(0)
  }

  /* test if the %TEMP% directory is writeable */
  if (trycreatefileindir(tempdir) != 0) {
    kitten_printf(2, 18, "ERROR: Unable to write in the '%s' directory. Check your %%TEMP%% variable.", tempdir);
    puts("");
    QUIT(0)
  }

  /* listing local packages need no special preparation - do it now and quit */
  if (action == ACTION_LISTLOCAL) {
    char *filterstr = NULL;
    if (argc == 3) filterstr = pkg;
    showinstalledpkgs(filterstr, dosdir);
    QUIT(0);
  }

  /* listing local files need no special preparation - do it now and quit */
  if (action == ACTION_LISTFILES) {
    listfilesofpkg(pkg, dosdir);
    QUIT(0);
  }

  /* if we install from a local file, do it and quit */
  if (action == ACTION_INSTALL_LOCALFILE) {
    char pkgname[64];
    char buffmem1k[1024];
    int t, lastpathdelim = -1, u = 0;
    for (t = 0; pkg[t] != 0; t++) {
      if ((pkg[t] == '/') || (pkg[t] == '\\')) lastpathdelim = t;
    }
    /* copy the filename into pkgname (without path elements) */
    for (t = lastpathdelim + 1; pkg[t] != 0; t++) pkgname[u++] = pkg[t];
    pkgname[u] = 0; /* terminate the string */
    /* truncate the file's extension (.zip) */
    for (t = u; t > 0; t--) {
      if (pkgname[t] == '.') {
        pkgname[t] = 0;
        break;
      }
    }

    /* prepare the zip file and install it */
    zipfileidx = pkginstall_preparepackage(pkgdb, pkgname, tempdir, pkg, flags, repolist, &zipfilefd, proxy, proxyport, downloadingstring, dosdir, dirlist, buffmem1k, mapdrv);
    if (zipfileidx != NULL) {
      pkginstall_installpackage(pkgname, dosdir, dirlist, zipfileidx, zipfilefd, mapdrv);
      fclose(zipfilefd);
    }

    QUIT(0)
  }

  /* sparky4: Reinstall a local file! */
  if (action == ACTION_REINSTALL_LOCALFILE) {
    char pkgname[64];
    char buffmem1k[1024];
    int t, lastpathdelim = -1, u = 0;
    for (t = 0; pkg[t] != 0; t++) {
      if ((pkg[t] == '/') || (pkg[t] == '\\')) lastpathdelim = t;
    }
    /* copy the filename into pkgname (without path elements) */
    for (t = lastpathdelim + 1; pkg[t] != 0; t++) pkgname[u++] = pkg[t];
    pkgname[u] = 0; /* terminate the string */
    /* truncate the file's extension (.zip) */
    for (t = u; t > 0; t--) {
      if (pkgname[t] == '.') {
        pkgname[t] = 0;
        break;
      }
    }

    /* prepare the zip file */
    zipfileidx = pkginstall_preparepackage(pkgdb, pkgname, tempdir, pkg, flags | PKGINST_UPDATE, repolist, &zipfilefd, proxy, proxyport, downloadingstring, dosdir, dirlist, buffmem1k, mapdrv);
    /* if the zip file is ok, remove the old package and install our zip file */
    if (zipfileidx != NULL) {
      if (pkgrem(pkgname, dosdir, mapdrv) != 0) { /* mayday! removal failed for some reason */
        zip_freelist(&zipfileidx);
      } else {
        pkginstall_installpackage(pkgname, dosdir, dirlist, zipfileidx, zipfilefd, mapdrv);
      }
      fclose(zipfilefd);
    }
    QUIT(0)
  }

  /* clear cache? */
  if (action == ACTION_CLEARCACHE) {
    char tempfile[512];
    sprintf(tempfile, "%s\\fdnpkg16.db", tempdir);
    unlink(tempfile);
    kitten_puts(2, 19, "Cache cleared.");
    QUIT(0);
  }

  /* if we want to play, check if any repo is configured */
  if (repolistcount == 0) {
    kitten_puts(2, 5, "No repository is configured. You need at least one.");
    kitten_puts(2, 6, "You should place in your configuration file at least one entry of such form:");
    puts("REPO http://www.freedos.org/repo");
    puts("");
    QUIT(0)
  }

  /* sparky4: check arg2 for a . if ther eis one in existance then skip networking initiation */
  if (!((pkg[arglen - 4] == '.') && (tolower(pkg[arglen - 3]) == 'z') && (tolower(pkg[arglen - 2]) == 'i'))) { /* if argument ends with '.zi?' (zip/zib), then it's a local package file */
    /* if there is at least one online repo, init the Watt32 stack */
    for (x = 0; x < repolistcount; x++) {
      if (detect_localpath(repolist[x]) == 0) {
        #ifdef USE_EXTERNAL_MTCP
        #ifdef DEBUG
        printf("mTCP is used\n");
        #endif /* #ifdef DEBUG */
        netinitres = system("dhcp");
        #else /* #ifdef USE_EXTERNAL_MTCP */
        #ifdef DEBUG
        printf("watt32 is used\n");
        #endif /* #ifdef DEBUG */
        netinitres = net_init();
        #endif /* #ifdef USE_EXTERNAL_MTCP */
        if (netinitres != 0) {
          kitten_puts(2, 15, "Error: TCP/IP initialization failed!");
          QUIT(0)
        }
        break;
      }
    }
  }

  if (action == ACTION_DUMPCFG) { /* if all we wanted was to print out repositories... */
    struct customdirs *dircursor;
    printf("maxcachetime: %ld seconds\n", maxcachetime);
    printf("installsources: %d\n", (flags & PKGINST_NOSOURCE) != 0);
    printf("mapdrives: %s\n", mapdrv);
    puts("");
    for (dircursor = dirlist; dircursor != NULL; dircursor = dircursor->next) {
      printf("%s -> %s\n", dircursor->name, dircursor->location);
    }
    if (dirlist != NULL) puts("");
    kitten_printf(2, 8, "The list of configured FDNPKG%s repositories follows:", EXECNAME); puts("");
    for (x = 0; x < repolistcount; x++) puts(repolist[x]);
    puts("");
  } else { /* other actions: search, install, checkupdates, update - all that require to load content of repositories */
    pkgdb = createdb();
    if (pkgdb != NULL) {
      char tempfilegz[512];
      char tempfile[512];
      char repoindex[512];
      int ungzres;
      sprintf(tempfile, "%s\\fdnpkg16.db", tempdir);
      if (loaddb_fromcache(pkgdb, tempfile, cfgfilecrc, maxcachetime) == 0) { /* load db from cache (if not older than 2h) */
        kitten_puts(2, 13, "Package database loaded from local cache.");
      } else {
        freedb(&pkgdb);      /* recreate the db from scratch, because after */
        pkgdb = createdb();  /* a cache attempt it might contain garbage */
        /* download every repo index into %TEMP% and load them in RAM */
        for (x = 0; x < repolistcount; x++) {
          kitten_printf(2, 16, "Loading %s...", repolist[x]);
          puts("");
          sprintf(repoindex, "%sindex.gz", repolist[x]);
          if (detect_localpath(repolist[x]) != 0) { /* if it's an ondisk repo, use the file as-is */
            strcpy(tempfilegz, repoindex);
            htgetres = 100; /* whatever > 0 would be fine, too */
          } else { /* else it's a network resource, let's download it */
            sprintf(tempfilegz, "%s\\fdnpkg_z.tmp", tempdir);
            #ifdef DEBUG
            puts("DEBUG: download start");
            #endif
            htgetres = 0;
            #ifdef USE_EXTERNAL_MTCP
            sprintf(command, "@echo off\nhtget -quiet -o %s %s", tempfilegz, repoindex);
            #endif
            for (y = 0; y < MAXINDEXRETRIES; y++) {
              sprintf(repoindex, "%sindex.gz", repolist[x]);  // refresh the index variable
              #ifdef DEBUG
              printf("\trepoindex: %s\n", repoindex);
              #endif
              #ifndef USE_EXTERNAL_MTCP
              if (htgetres > 0) break;
              htgetres = http_get(repoindex, tempfilegz, proxy, proxyport, NULL);
              #else
              if (htgetres == 21) break;
              // lets try this
              sprintf(commandforbatch, "%s\\fdnpkg16.bat", tempdir);
              batch_file = fopen(commandforbatch, "w");
              if (batch_file == NULL) {
                kitten_printf(3, 10, "Error: Could not create %s!");
                htgetres = -1;
              } else {
                fprintf(batch_file, "%s", command);
                fclose(batch_file);
                _heapmin();
                _heapshrink(); // sparky4: these 2 functions are for heap management to make it smaller so we can call the batch file with the commands
                htgetres = system(commandforbatch);
              }
              #endif
              #ifdef DEBUG
              printf("htgetres returned: %d\n", htgetres);
              #endif
              #ifndef USE_EXTERNAL_MTCP
              if (htgetres <= 0) putchar('.');
              #else
              if (htgetres != 21) putchar('.');
              #endif
            }
            #ifdef DEBUG
            puts("DEBUG: download stop");
            #endif
          }
          #ifndef USE_EXTERNAL_MTCP
          if (htgetres <= 0) {
            #else
            if (htgetres != 21) {
              #endif
              kitten_puts(2, 10, "Repository download failed!");
              #ifndef ERRCACHE
              maxcachetime = 0; /* disable cache writing this time */
              #endif
          } else {
            char *dbmsg;
            /* uncompress and load the index file */
            sprintf(tempfile, "%s\\fdnpkg16.tmp", tempdir);
            ungzres = ungz(tempfilegz, tempfile);
            if ((ungzres != 0) || (loaddb(pkgdb, tempfile, x, &dbmsg) != 0)) {
              kitten_puts(2, 11, "An error occured while trying to load repository from tmp file...");
              printf("Error code: %d\n", ungzres);
              maxcachetime = 0; /* disable cache writing this time */
            }
            /* if there's a message to display from the db provider, display it now */
            if (dbmsg != NULL) {
              puts("");
              kitten_printf(6, 5, "Message from %s:", repolist[x]);
              printf("\n%s\n", dbmsg);
              free(dbmsg);
            }
            if (htgetres > 0) puts("ok"); // just let the user know the file was downloaded and installed
          }
        }
        /* save results into the (new) cache file db */
        if (maxcachetime > 0) {
          sprintf(tempfile, "%s\\fdnpkg16.db", tempdir);
          dumpdb(pkgdb, tempfile, cfgfilecrc);
        }
      }
      if (action == ACTION_SEARCH) { /* for search: iterate through the sorted db, and print out all packages that match the pattern */
        if (argc == 3) { /* a search term has been provided */
          pkgsearch(pkgdb, pkg, verbosemode, repolist);
        } else {
          pkgsearch(pkgdb, NULL, verbosemode, repolist);
        }
      } else if (action == ACTION_INSTALL) {
        if (validate_package_not_installed(pkg, dosdir, mapdrv) == 0) { /* check that package is not already installed first */
          char membuff1k[1024];
          zipfileidx = pkginstall_preparepackage(pkgdb, pkg, tempdir, NULL, flags, repolist, &zipfilefd, proxy, proxyport, downloadingstring, dosdir, dirlist, membuff1k, mapdrv);
          if (zipfileidx != NULL) {
            pkginstall_installpackage(pkg, dosdir, dirlist, zipfileidx, zipfilefd, mapdrv);
            fclose(zipfilefd);
          }
        }
      } else if (action == ACTION_UPDATE) { /* UPDATE, but only for a SINGLE package */
        if (is_package_installed(pkg, dosdir, mapdrv) == 0) { /* is this package installed at all? */
          kitten_printf(10, 6, "Package %s is not installed.", pkg);
          puts("");
        } else if (checkupdates(dosdir, pkgdb, repolist, pkg, tempdir, 0, dirlist, proxy, proxyport, downloadingstring, mapdrv) != 0) { /* no update available */
          kitten_printf(10, 2, "No update found for the '%s' package.", pkg);
          puts("");
        } else { /* the package is locally installed, and an update have been found - let's proceed */
          char membuff1k[1024];
          /* prepare the zip file */
          zipfileidx = pkginstall_preparepackage(pkgdb, pkg, tempdir, NULL, flags | PKGINST_UPDATE, repolist, &zipfilefd, proxy, proxyport, downloadingstring, dosdir, dirlist, membuff1k, mapdrv);
          /* if the zip file is ok, remove the old package and install our zip file */
          if (zipfileidx != NULL) {
            if (pkgrem(pkg, dosdir, mapdrv) != 0) { /* mayday! removal failed for some reason */
              zip_freelist(&zipfileidx);
            } else {
              pkginstall_installpackage(pkg, dosdir, dirlist, zipfileidx, zipfilefd, mapdrv);
            }
            fclose(zipfilefd);
          }
        }
      } else if (action == ACTION_UPGRADE) { /* recursive UPDATE for the whole system */
        checkupdates(dosdir, pkgdb, repolist, NULL, tempdir, flags | PKGINST_UPDATE, dirlist, proxy, proxyport, downloadingstring, mapdrv);
      } else if (action == ACTION_CHECKUPDATES) { /* checkupdates */
        checkupdates(dosdir, pkgdb, repolist, NULL, tempdir, 0, dirlist, proxy, proxyport, downloadingstring, mapdrv);
      } else if (action == ACTION_REINSTALL) { /* REINSTALL, but only for a SINGLE package */
        char membuff1k[1024];
        /* prepare the zip file */
        zipfileidx = pkginstall_preparepackage(pkgdb, pkg, tempdir, NULL, flags | PKGINST_UPDATE, repolist, &zipfilefd, proxy, proxyport, downloadingstring, dosdir, dirlist, membuff1k, mapdrv);
        /* if the zip file is ok, remove the old package and install our zip file */
        if (zipfileidx != NULL) {
          if (pkgrem(pkg, dosdir, mapdrv) != 0) { /* mayday! removal failed for some reason */
            zip_freelist(&zipfileidx);
          } else {
            pkginstall_installpackage(pkg, dosdir, dirlist, zipfileidx, zipfilefd, mapdrv);
          }
          fclose(zipfilefd);
        }
      }
      /* free memory of the pkg database */
      freedb(&pkgdb);
    } /* pkgdb != NULL */
  } /* action == ACTION_LISTREP */
  } /* sparky4: end of that huge for loop. This loop manages the packages in the argument list! :D */
  freeconf(repolist, repolistcount, &dirlist);
  QUIT(0);
}
