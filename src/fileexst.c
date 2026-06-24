/*
 * fileexists checks whether a file exists or not.
 * returns 0 if file not found, non-zero otherwise.
 */

#include <stdio.h>
#include "fileexst.h"
#include "version.h"

int fileexists(const char *filename) {
  FILE *fd;
  fd = fopen(filename, "rb");
  if (fd != NULL) { /* file exists */
      fclose(fd);
      return(1);
    } else {
      return(0);
  }
}

long filesize(const char *filename) {
  long size;
  FILE *fp;
  fp = fopen(filename, "rb");
  if (fp == NULL) {
    fclose(fp);
    return -1;
  } else {
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    fclose(fp);
  }
  return(size);
}
