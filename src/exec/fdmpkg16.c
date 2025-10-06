/*
 * FDMPKG16 - FreeDOS Network Package manager Caller for Multi Packages
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
#include <stdio.h>
#include <stdlib.h>    /* system() */
#include <malloc.h>    /* _heapmin() */
//#include <string.h>    /* strcpy() */
//#include <strings.h>   /* strcasecmp() */
//#include <conio.h>     /* getch() */

#include "fdnpkg16.h"  /*  */
#include "version.h"

void main(int argc, char *argv[]) {
  int i;
  static char command[48];

  if (argc < 3) {
#ifndef USE_EXTERNAL_MTCP
    printf("FDMPKG16 is used to install more than 1 package! (This program is temporary) :D\nJust pass 2 or more arguments!\nExample: FDMPKG16 ri fdnpkg16 fdnpkg...\n");
#else
    printf("FDMPKG86 is used to install more than 1 package! (This program is temporary) :D\nJust pass 2 or more arguments!\nExample: FDMPKG86 ri fdnpkg16 fdnpkg...\n");
#endif
    return;
  }
  for (i = 2; i < argc; i++) {
#ifndef USE_EXTERNAL_MTCP
    sprintf(command, "fdnpkg16.exe %s %s", argv[1], argv[i]);
#else
    sprintf(command, "fdnpkg86.exe %s %s", argv[1], argv[i]);
#endif
    printf("command: %s\n", command);// getch();
    _heapmin();
    _heapshrink(); // sparky4: these 2 functions are for heap management to make it smaller so we can call the batch file with the commands
    system(command);
  }
}
