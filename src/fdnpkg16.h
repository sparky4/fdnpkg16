/*
 *  This file is part of FDNPKG
 *  Copyright (C) 2012-2016 Mateusz Viste
 */

#ifndef FDNPKG_H_SENTINEL
#define FDNPKG_H_SENTINEL

/* flags used by FDNPKG */
#define PKGINST_NOSOURCE  1
#define PKGINST_SKIPLINKS 2
#define PKGINST_UPDATE    4
 #define DEBUG  /* uncomment this to enable debug mode */
//#define USE_EXTERNAL_MTCP   // enable this to use the external mtcp or internal watt32
#define ERRCACHE            // enable this to have databases, always

struct flist_t {
  struct flist_t *next;
  char fname[2]; /* this must be the last item in the structure, it will be expanded to fit the filename */
};

#endif
