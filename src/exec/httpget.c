/*
 * test app for the FDNPKG16 http module.
 * Copyright (C) Mateusz Viste 2013-2015. All rights reserved.
 * Copyright (C) Victoria Crenshaw (sparky4) 2025-2026. All rights reserved
 */

#include <stdio.h>
#include <strings.h>
#include "http.h"
#include "net.h"

//#define VERBOSE
//#define DEBUG
#ifdef DEBUG
#include <malloc.h>
//#include "dbg/dbg.h"

long coreleft()
{
  _nheapgrow();
  return _memavl();
}

// from: https://forum.vcfed.org/index.php?threads/ibm-5160-memory-management-c-code-compiling-with-open-watcom.1247002/post-1369076
// sparky4: modified by me xD
long farcoreleft() {
  static long memoryAvailable;
  int SAMPLE_SIZE = 640;  /* 640 Bytes */
  void __far * far *memoryBlock; /* Array of pointers */
  int i = 0, j = 0;
  int maxBlocks = 1024; /* for a max of about 640KB */

  _fheapgrow();
  /* Allocate memory for pointers */
  memoryBlock = _fmalloc(maxBlocks * sizeof(void*));
  if (memoryBlock == NULL) {
      /* Handle allocation failure of memoryBlock */
      return -1;
  }
  /* Allocate 60 chunks */
  for (i = 0; i < maxBlocks; i++) {
      memoryBlock[i] = _fmalloc(SAMPLE_SIZE);
      if (memoryBlock[i] == NULL) {
          break; /* end of available memory reached */
      }
  }

  /* Calculate total memory allocated successfully */
  memoryAvailable = (long)SAMPLE_SIZE * (long)i;

//  printf("Memory available: %ld %d\n", memoryAvailable, i);

  /* Free everything */
  for (j = 0; j < i; j++) {
      _ffree(memoryBlock[j]); /* Free each allocated block */
      memoryBlock[j] = NULL;
  }
  /* Free the array of pointers itself */
  _ffree(memoryBlock);
  memoryBlock = NULL;
  _fheapshrink();
  return memoryAvailable;
}
#endif  // #ifdef DEBUG

// main
int main(int argc, char **argv) {
  long res;
  if (argc < 3) {
    printf("%s url outfile arguments... [/q]", argv[0]);
    return(1);
  }
  res = net_init();
  if (res != 0) {
    printf("net_init() error: %ld\n", res);
    return(1);
  }
#ifdef DEBUG
  printf("farcoreleft() == %ld\n", farcoreleft());
  printf("coreleft() == %u\n", coreleft());
#endif
  if (strcasecmp(argv[3], "/q")) {
    res = http_get(argv[1], argv[2], NULL, 8080, "Downloading %s... %ld bytes", 1);
  } else {
    res = http_get(argv[1], argv[2], NULL, 8080, '\0', 0);
  }
#ifdef DEBUG
  printf("farcoreleft() == %ld\n", farcoreleft());
  printf("coreleft() == %u\n", coreleft());
#endif
  if (res >= 0) {
#ifdef VERBOSE
    if ((strcasecmp(argv[3], "/q"))) {
      printf("Saved %ld bytes into %s.\n", res, argv[2]);
    }
#endif
  } else {
    printf("ERROR OCCURED: %ld\n", res);
  }
  return(0);
}
