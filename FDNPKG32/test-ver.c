/*
 * This file is part of FDNPKG.
 * Copyright (C) Mateusz Viste 2013-2015. All rights reserved
 *
 * Tests two version strings, and tells if the second one is newer than the first one.
 */

#include <stdio.h>
#include "helpers.h"

int main(int argc, char **argv) {
  int x;
  if (argc != 3) {
    printf("usage: %s ver1 ver2\n", argv[0]);
    return(1);
  }
  x = isversionnewer(argv[1], argv[2]);
  if (x > 0) {
    printf("'%s' is newer than '%s'.\n", argv[2], argv[1]);
  } else if (x == 0) {
    printf("'%s' is NOT newer than '%s'.\n", argv[2], argv[1]);
  } else {
    printf("could not understand version string(s).\n");
  }
  return(0);
}
