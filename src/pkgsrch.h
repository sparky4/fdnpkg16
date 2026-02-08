/*
 * This file is part of FDNPKG16
 * Copyright (C) 2012-2016 Mateusz Viste
 * Copyright (C) 2025-2026 Victoria Crenshaw aka sparky4 && Katheryn Northwood
 */

#ifndef pkgsrch_sentinel
#define pkgsrch_sentinel

#include "pkgdb.h"
void pkgsearch(struct pkgdb *pkgdb, char *searchterm, int verbosemode, char **repolist);

#endif
