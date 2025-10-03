/*
 * test app for the FDNPKG http module.
 * Copyright (C) Mateusz Viste 2013-2015. All rights reserved.
 */

#include <stdio.h>
#include "http.h"
#include "net.h"

int main(int argc, char **argv) {
  int res;
  if (argc != 3) {
    printf("%s url outfile", argv[0]);
    return(1);
  }
  res = net_init();
  if (res != 0) {
    printf("net_init() error: %d\n", res);
    return(1);
  }
  res = http_get(argv[1], argv[2], NULL, 8080, "Downloading %s... %ld bytes");
  if (res >= 0) {
    printf("Saved %d bytes into %s.\n", res, argv[2]);
  } else {
    printf("ERROR OCCURED: %d\n", res);
  }
  return(0);
}
