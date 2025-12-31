/*
 * This file is part of FDNPKG16.
 *
 * Reads environment variables that will be used by FDNPKG and FDINST.
 * Returns 0 on success, non-zero otherwise.
 *
 * Copyright (C) 2012-2016 Mateusz Viste
 * Copyright (C) 2025-2026 Victoria Crenshaw aka sparky4
 */

#ifndef READENV_H_SENTINEL
#define READENV_H_SENTINEL

int readenv(char** dosdir, char** tempdir, char* cfgfile, int cfgfilemaxlen, char** argv);

#endif
