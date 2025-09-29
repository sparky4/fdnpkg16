/*
 * This file is part of the FDNPKG project.
 * Copyright (C) 2013-2016 Mateusz Viste
 */

#ifndef parseurl_h_sentinel
  #define parseurl_h_sentinel

  #define PARSEURL_PROTO_GOPHER 1
  #define PARSEURL_PROTO_HTTP 2
  #define PARSEURL_PROTO_FTP 3
  #define PARSEURL_PROTO_UNKNOWN -1

  /* explodes a URL into parts, and return the protocol id, or a negative value on error */
  int parseurl(char *url, char *host, int *port, char *path);

#endif
