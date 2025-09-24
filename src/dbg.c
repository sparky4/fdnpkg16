/*
 * debugging module for FDNPKG, to track fopen() vs. fclose() calls, as well
 * as malloc/calloc() vs free() calls.
 *
 * Copyright (C) 2015-2016 Mateusz Viste
 */

#include <stdio.h>
#include <stdlib.h>

static long fopencount = 0;
static long fclosecount = 0;
static long malloccount = 0;
static long calloccount = 0;
static long freecount = 0;

FILE *dbg_fopen(char *fname, char *mode) {
  FILE *fd;
  fd = fopen(fname, mode);
  if (fd != NULL) fopencount++;
  return(fd);
}

int dbg_fclose(FILE *fd) {
  fclosecount++;
  return(fclose(fd));
}

void *dbg_malloc(long sz) {
  unsigned char *res;
  res = malloc(sz);
  if (res != NULL) malloccount++;
  return(res);
}

void *dbg_calloc(long sz, long n) {
  unsigned char *res;
  res = calloc(sz, n);
  if (res != NULL) calloccount++;
  return(res);
}

void dbg_free(void *ptr) {
  if (ptr != NULL) {
    free(ptr);
    freecount++;
  }
}

void dbg_printstats(void) {
  printf("fopen calls: %ld / fclose calls: %ld\nmalloc() calls: %ld / calloc() calls: %ld / free() calls: %ld\n", fopencount, fclosecount, malloccount, calloccount, freecount);
}
