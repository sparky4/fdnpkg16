/*
 * This file is part of FDNPKG16
 * Copyright (C) 2013-2016 Mateusz Viste, All rights reserved.
 * Copyright (C) 2025-2026 Victoria Crenshaw aka sparky4
 */

#ifndef readlsm_h_sentinel
  #define readlsm_h_sentinel
  /* Loads metadata from an LSM file. Returns 0 on success, non-zero on error. */
  int readlsm(char *filename, char *version, int version_maxlen);
#endif
