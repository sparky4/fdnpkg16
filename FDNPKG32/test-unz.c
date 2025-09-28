/*
 * This file is part of FDNPKG
 * Copyright (C) Mateusz Viste 2013. All rights reserved.
 * 
 * Test program for libunzip.
 */

#include <dpmi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>     /* ctime() */
#include "crc32.h"
#include "libunzip.h"
#include "lzmadec.h"

unsigned _stklen = 1048576; /* we want 1M of stack space */


int main(int argc, char **argv) {
  char *zipfile;
  struct ziplist *zipindex, *zipcursor;
  int filescount = 0;
  FILE *fd;

  printf("Available physical mem: %lu KiB\nAvailable virtual mem: %lu KiB\n\n", _go32_dpmi_remaining_physical_memory() / 1024, _go32_dpmi_remaining_virtual_memory() / 1024);

  if (argc != 2) {
    puts("Usage: test-unz file.zip");
    return(1);
  }

  zipfile = argv[1];

  fd = fopen(zipfile ,"rb");
  if (fd == NULL) {
    puts("zip file could not be opened!");
    return(3);
  }

  zipindex = zip_listfiles(fd);

  if (zipindex == NULL) {
    puts("Error: could not parse the zip file!");
    return(2);
  }

  for (zipcursor = zipindex; zipcursor != NULL; zipcursor = zipcursor->nextfile) {

    printf("%s [%ld->%ld]/meth #%d/CRC %08lX/offset 0x%lX/Dir=%d/Enc=%d\n", zipcursor->filename, zipcursor->filelen, zipcursor->compressedfilelen, zipcursor->compmethod, zipcursor->crc32, zipcursor->dataoffset, zipcursor->flags & ZIP_FLAG_ISADIR, zipcursor->flags & ZIP_FLAG_ENCRYPTED);
    printf("  timestamp: %s\n", ctime(&zipcursor->timestamp));

    filescount += 1;

  }
  printf("Total files: %d\n", filescount);

  fclose(fd);
  zip_freelist(&zipindex);

  return(0);
}
