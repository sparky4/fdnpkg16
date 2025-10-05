/*
 * This file is part of FDNPKG
 * Copyright (C) Mateusz Viste 2013. All rights reserved.
 *
 * Test program for libgz.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>     /* ctime() */
#include "libgz.h"


int main(int argc, char **argv) {
  char *gzfile, *outfile;
  int result;
  FILE *fd;

  if (argc != 3) {
    puts("Usage: test-gz file.gz outfile");
    return(1);
  }

  gzfile = argv[1];
  outfile = argv[2];

  fd = fopen(gzfile, "rb");
  if (fd == NULL) {
    puts("gz file could not be opened!");
    return(3);
  }

  puts("outfile opened OK");

  result = ungz(fd, outfile);

  if (result != 0) printf("ungz() failed with code %d\n", result);

  fclose(fd);

  return(0);
}
