/*
 * This file is part of the FDNPKG project
 * Copyright (C) Mateusz Viste 2013
 *
 * Simple test program for the FDNPKG url parser.
 */

#include <stdio.h>
#include "parseurl.h"

int main(int argc, char **argv) {
  char host[1024];
  char path[1024];
  int port;
  int proto;

  if (argc != 2) {
    puts("usage: test-url url");
    return(1);
  }

  proto = parseurl(argv[1], host, &port, path);
  printf("Prot: %d\n", proto);
  printf("Host: '%s'\n", host);
  printf("Port: %d\n", port);
  printf("Path: '%s'\n", path);

  return(0);
}
