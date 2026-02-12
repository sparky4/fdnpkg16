/*
 * test app for the FDNPKG16 http module.
 * Copyright (C) Mateusz Viste 2013-2015. All rights reserved.
 * Copyright (C) Victoria Crenshaw (sparky4) && Katheryn Northwood 2025-2026. All rights reserved.
 */
/*
 * HTTPGET.EXE part of FDNPKG16.
 * Currently in use as the networking part of FDNPKG16 to get the packages.
 * Can be used as stand alone software
 */

#include <stdio.h>
#include <strings.h>
#include <unistd.h>   /* unlink() */

//#define DEBUG_NET
//#define VERBOSE_HTTPGET
//#define DEBUG_HTTPGET

#include "http.h"
#include "net.h"
#include "memcore.h"

#ifdef DEBUG_HTTPGET
#define DEBUG
#include <malloc.h>
//#include "dbg/dbg.h"

#if 0
// sparky4: This function I did make. It works well. Returns near memory avalible!
long coreleft() {
  _nheapgrow();
  return _memavl();
}

// from: https://forum.vcfed.org/index.php?threads/ibm-5160-memory-management-c-code-compiling-with-open-watcom.1247002/post-1369076
// sparky4: modified by me xD
long farcoreleft() {
  static long memoryAvailable;
  int SAMPLE_SIZE = 640;//65534;  /* 640 Bytes */
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
#endif  // #ifdef DEBUG_HTTPGET
#endif

// main
long main(int argc, char **argv) {
  long res;
  char downloadingstring[64];
#ifdef DEBUG_HTTPGET
  long memoryeaten;
#endif
  if (argc < 2) {
    printf("%s url outfile arguments... [/q]", argv[0]);
    return(1);
  }
  res = net_init();
  if (res != 0) {
    printf("net_init() error: %ld\n", res);
    return(1);
  }
#ifdef DEBUG_HTTPGET
  memoryeaten = farcoreleft();
  printf("farcoreleft() == %ld\n", farcoreleft());
  printf("coreleft() == %u\n", coreleft());
#endif
  if ((argv[3] == NULL)) {
    sprintf(downloadingstring, "%s", "Downloading %s... %ld bytes");
  } else if (argv[4] != NULL) {
    sprintf(downloadingstring, "%s", argv[4]);
  }
  if (strcasecmp(argv[1], "fcl") == 0) {
    printf("farcoreleft() == %ld Byte(s) Free\n", farcoreleft());
    printf("coreleft() == %ld Byte(s) Free\n", coreleft());
    return(0);
  } else if (strcasecmp(argv[3], "/q") == 0) {
    res = http_get(argv[1], argv[2], NULL, 8080, '\0', 0);
  } else {
    res = http_get(argv[1], argv[2], NULL, 8080, downloadingstring, 1);
  }
#ifdef DEBUG_HTTPGET
  memoryeaten -= farcoreleft();
  printf("farcoreleft() == %ld\n", farcoreleft());
  printf("coreleft() == %u\n", coreleft());
#endif
  if (res >= 0) {
#ifdef VERBOSE_HTTPGET
    if (strcasecmp(argv[3], "/q") != 0) {
      printf("Saved %ld bytes into %s.\n", res, argv[2]);
    }
#endif
  } else {
    printf("ERROR OCCURED: %ld\n", res);
    if ((argv[3] != NULL) && (argv[4] != NULL)) {
      //printf("Deleting %s\n", argv[2]);
      unlink(argv[2]);
    }
  }
#ifdef DEBUG_HTTPGET
  printf("memory eaten == %ld\n", memoryeaten);
#endif
  return(res);
}
