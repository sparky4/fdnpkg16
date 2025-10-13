/*
 * Version strings used by both FDNPKG and FDINST, and possibly other common
 * definitions. This file should be included by every single module.
 *
 * http://fdnpkg.sourceforge.net
 * Copyright (C) 2012-2021 Mateusz Viste
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

#ifndef COMMON_H_SENTINEL
#define COMMON_H_SENTINEL

#include <stdio.h>

#define PVER "0.99.8248c"
#define PDATE "2012-2021 && 2025"

/* uncomment the line below to enable fopen/fclose and malloc/free stats. you
 * will also have to link the application with dbg.o, and call dbg_stats()
 * just before the application returns to the OS. */
// #include "dbg.h"

#endif
