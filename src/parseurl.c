/*
 * This file is part of the FDNPKG project.
 * Copyright (C) 2013-2016 Mateusz Viste
 */

#include <string.h>   /* strstr() */
#include <stdlib.h>   /* atoi() */

#include "version.h"
#include "parseurl.h" /* include self for control */


/* Parses an URL and returns the protocol used, as well as auxiliary data like port, path... */
int parseurl(char *url, char *host, int *port, char *selector) {
  int parserstate = 0;
  int protocol = PARSEURL_PROTO_HTTP, x;
  char *curtoken;
  char itemtypedata;
  char *itemtype = &itemtypedata;
  /* set default values */
  *port = 80;
  *itemtype = '1';
  *selector = 0;
  /* skip the protocol part, if present */
  for (x = 0; url[x] != 0; x++) {
    if (url[x] == '/') { /* no protocol */
      protocol = PARSEURL_PROTO_HTTP;  /* if no protocol explicitely given, assume http */
      break;
    }
    if (url[x] == ':') { /* found a colon. check if it's for proto declaration */
      if (url[x + 1] == '/') {
        if (url[x + 2] == '/') {
          char *protostr = url;
          url[x] = 0;
          url += x + 3;
          if (strcasecmp(protostr, "gopher") == 0) {
            protocol = PARSEURL_PROTO_GOPHER;
            *port = 70; /* default port is 70 for Gopher */
          } else if (strcasecmp(protostr, "http") == 0) {
            protocol = PARSEURL_PROTO_HTTP;
            *port = 80; /* default port is 80 for HTTP */
            *itemtype = 'h';
          } else {
            protocol = PARSEURL_PROTO_UNKNOWN;
          }
          break;
        }
      }
      protocol = PARSEURL_PROTO_HTTP;
      break;
    }
  }
  /* start reading the url */
  curtoken = url;
  for (; parserstate < 4; url += 1) {
    switch (parserstate) {
      case 0:  /* reading host */
        if (*url == ':') { /* a port will follow */
          *host = 0;
          curtoken = url + 1;
          parserstate = 1;
        } else if (*url == '/') { /* gopher type will follow */
          *host = 0;
          parserstate = 2;
        } else if (*url == 0) { /* end of url */
          *host = 0;
          parserstate = 4;
        } else { /* still part of the host */
          *host = *url;
          host += 1;
        }
        break;
      case 1:  /* reading port */
        if (*url == 0) { /* end of url */
          *port = atoi(curtoken);
          parserstate = 4;
        } else if (*url == '/') {
          *url = 0; /* temporary end of string */
          *port = atoi(curtoken);
          *url = '/'; /* restore the original char */
          parserstate = 2; /* gopher type follows */
        }
        break;
      case 2:  /* reading itemtype */
        if (protocol == PARSEURL_PROTO_GOPHER) { /* if non-Gopher, skip the itemtype */
          if (*url != 0) {
            *itemtype = *url;
            parserstate = 3;
          } else {
            parserstate = 4;
          }
          url += 1;
        }
        parserstate = 3; /* go right to the url part now */
      case 3:
        if (*url == 0) {
          *selector = 0;
          parserstate = 4;
        } else {
          *selector = *url;
          selector += 1;
        }
        break;
    }
  }
  return(protocol);
}
