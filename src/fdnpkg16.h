/*
 *  This file is part of FDNPKG16
 *  Copyright (C) 2012-2016 Mateusz Viste
 *  Copyright (C) 2025-2026 Victoria Crenshaw aka sparky4 && Katheryn Northwood
 */

#ifndef FDNPKG16_H_SENTINEL
#define FDNPKG16_H_SENTINEL

/* flags used by FDNPKG16 */
#define PKGINST_NOSOURCE  1
#define PKGINST_SKIPLINKS 2
#define PKGINST_UPDATE    4

#define FDNPKG16_NETINIT  8   // sparky4: no network action
#define FDNPKG16_NOREPOA  16  // sparky4: no repository action
#define FDNPKG16_NOINST   32  // sparky4: no install action

#define MAXINDEXRETRIES   10  // maximum number of retries for downloading a repository index
//#define DEBUG                 // uncomment this to enable debug mode


//old dont use yet
//#define USE_INTERNAL_WATTCP   // enable/disable this to use external or internal watt32
//#define USE_MTCP              // enable/disable this to use mtcp or watt32 externally

struct flist_t {
  struct flist_t *next;
  char fname[2]; /* this must be the last item in the structure, it will be expanded to fit the filename */
};

#endif
