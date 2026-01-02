/*
 * This file is part of FDNPKG16
 * Copyright (C) 2013-2017 Mateusz Viste
 * Copyright (C) 2025-2026 Victoria Crenshaw aka sparky4
 */

#include "fdnpkg16.h"
#include "pkgdb.h"

#ifndef showinst_h_sentinel
  #define showinst_h_sentinel
  void pkg_freeflist(struct flist_t *flist);
  struct flist_t *pkg_loadflist(char *pkgname, char *dosdir);
  void showinstalledpkgs(char *filterstr, char *dosdir);
  void showheldedpkgs(char *filterstr, char *dosdir);
  int checkupdates(char *dosdir, struct pkgdb *pkgdb, char **repolist, char *pkg, char *tempdir, int flags, struct customdirs *dirlist, char *proxy, int proxyport, char *downloadingstring, char *mapdrv);
  void listfilesofpkg(char *pkgname, char *dosdir);
  void holdpkg(char *pkgname, char *dosdir);
  void unholdpkg(char *pkgname, char *dosdir);
#endif
