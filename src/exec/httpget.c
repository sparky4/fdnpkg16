/*
 * test app for the FDNPKG16 http module.
 * Copyright (C) Mateusz Viste 2013-2015. All rights reserved.
 * Copyright (C) Victoria Crenshaw (sparky4) && Katheryn Northwood 2025-2026. All rights reserved.
 */
/*
 * HTTPGET.EXE part of FDNPKG16.
 * Currently in use as the networking part of FDNPKG16 to get the packages.
 * Can be used as stand alone software.
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
#endif  // #ifdef DEBUG_HTTPGET

// main
long main(int argc, char **argv) {
  long res;
  char downloadingstring[64];
  extern char *wattcpVersion(); /* provided by wattcp to poll its version */
#ifdef DEBUG_HTTPGET
  long memoryeaten;
#endif
  if (argc < 2) {
    printf("%s url outfile arguments... [/q]\n", argv[0]);
    puts("");
    printf("HTTPGET.EXE is linked against the Watt-32 version below:");
    puts("");
    puts(wattcpVersion());
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
