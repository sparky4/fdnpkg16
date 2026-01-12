/*
 * sparky4: The code here is mostly from the web.
 * I dont think it is subject to copyright.
 */
/*
  heap check
*/

#include "memcore.h"
#include <malloc.h>

// sparky4: This function I did make. It works well. Returns near memory avalible!
long coreleft() {
  _nheapgrow();
  return _memavl();
}

// from: https://forum.vcfed.org/index.php?threads/ibm-5160-memory-management-c-code-compiling-with-open-watcom.1247002/post-1369076
long nearcoreleft() {
  static long memoryAvailable;
  int SAMPLE_SIZE = 640;  /* 640 Bytes */
  void **memoryBlock; /* Array of pointers */
  int i = 0, j = 0;
  int maxBlocks = 100; /* for a max of about 64KB */

  _nheapgrow();
  /* Allocate memory for pointers */
  memoryBlock = malloc(maxBlocks * sizeof(void*));
  if (memoryBlock == NULL) {
      /* Handle allocation failure of memoryBlock */
      return -1;
  }
  /* Allocate 60 chunks */
  for (i = 0; i < maxBlocks; i++) {
      memoryBlock[i] = malloc(SAMPLE_SIZE);
      if (memoryBlock[i] == NULL) {
          break; /* end of available memory reached */
      }
  }

  /* Calculate total memory allocated successfully */
  memoryAvailable = (long)SAMPLE_SIZE * (long)i;

//  cprintf("Memory available: %lu", memoryAvailable);

  /* Free everything */
  for (j = 0; j < i; j++) {
      free(memoryBlock[j]); /* Free each allocated block */
      memoryBlock[j] = NULL;
  }
  /* Free the array of pointers itself */
  free(memoryBlock);
  memoryBlock = NULL;
  _nheapshrink();
  return memoryAvailable;
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
