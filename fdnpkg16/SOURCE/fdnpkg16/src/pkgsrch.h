/*
 * This file is part of FDNPKG
 * Copyright (C) 2012-2016 Mateusz Viste
 */

#ifndef pkgsrch_sentinel
#define pkgsrch_sentinel

#include "pkgdb.h"
void pkgsearch(struct pkgdb *pkgdb, char *searchterm, int verbosemode, char **repolist);

#endif
