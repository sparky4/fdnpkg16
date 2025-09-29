/*
 * This file is part of the FDNPKG project.
 * Copyright (C) Mateusz Viste 2013-2015
 *
 * Provides a simplistic HTTP / Gopher downloader.
 */

#include <stdio.h>
#include <stdlib.h>   /* malloc() */
#include <string.h>   /* strlen() */
#include <time.h>

#include "helpers.h"  /* fdnpkg_strcasestr() */
#include "net.h"
#include "parseurl.h"
#include "http.h"  /* adding self for control */
#include "version.h"

#define buffer_max 4096


/* a function to fetch the next line from the socket  */
static int sock_getline(struct net_tcpsocket *socket, char *buf, int len) {
  int  i = 0, net_result;
  char ch;
  /* int status = 0;
  int *statusptr = &status; */
  while (i < len) {
    net_result = net_recv(socket, &ch, 1);
    if (net_result == 0) continue;  /* if got nothing, loop again */
    if (net_result < 0) break; /* conn error */
    if (ch == '\n') break;
    if (ch != '\r') {
      *buf++ = ch;
      i++;
    }
  }
  *buf = '\0';
  return(i);
}


/* returns 0 on success, non-zero otherwise
   Warning: Requires net_init() to be run before! */
int http_get(char *orgurl, char *outputfile, char *proxy, int proxyport, char *downloadingstring) {
  char *host = NULL;
  char *path = NULL;
  char *buffer = NULL;
  int port, result = -1, proto;
  unsigned long ipaddr;
  int x;
  long fdlen = 0, byteread = 0, expectedfilelen = -1;
  struct net_tcpsocket *socket;
  FILE *fd;
  time_t curtime, lastrefreshtime;
  char *shorturlfilename;  /* used for displaying */

  /* validate input data */
  if ((orgurl == NULL) || (outputfile == NULL)) return(-1);

  /* create a pointer to the short file name in the url */
  shorturlfilename = orgurl;
  for (x = 0; orgurl[x] != 0; x++) if (orgurl[x] == '/') shorturlfilename = &orgurl[x + 1];

  lastrefreshtime = time(NULL) + 1;

  for (;;) { /* not a real loop, just a container allowing to break out easily */
    x = strlen(orgurl) + 1;
    host = malloc(x);
    if (host == NULL) return(-2);
    path = malloc(x);
    if (path == NULL) break;
    buffer = malloc(buffer_max);
    if (buffer == NULL) break;

    proto = parseurl(orgurl, host, &port, path);
    if ((proto != PARSEURL_PROTO_HTTP) && (proto != PARSEURL_PROTO_GOPHER)) break;

    /* ignore proxy settings for any non-http protocols */
    if (proto != PARSEURL_PROTO_HTTP) proxy = NULL;

    if (proxy != NULL) {
      ipaddr = net_dnsresolve(proxy);
    } else {
      ipaddr = net_dnsresolve(host);
    }
    if (ipaddr == 0) break;

    /* printf("%s -> %lu.%lu.%lu.%lu (%d)\n", host, (ipaddr >> 24) & 0xFF, (ipaddr >> 16) & 0xFF, (ipaddr >> 8) & 0xFF, ipaddr & 0xFF, port); */

    if (proxy != NULL) {
      socket = net_connect(ipaddr, proxyport);
    } else {
      socket = net_connect(ipaddr, port);
    }

    if (socket == NULL) break;

    if (proto == PARSEURL_PROTO_HTTP) {
      if (proxy != NULL) {
        sprintf(buffer, "GET http://%s:%d/%s HTTP/1.1\r\nHOST: %s\r\nCONNECTION: close\r\nUSER-AGENT: FDNPKG\r\n\r\n", host, port, path, host);
      } else {
        sprintf(buffer, "GET /%s HTTP/1.1\r\nHOST: %s\r\nCONNECTION: close\r\nUSER-AGENT: FDNPKG\r\n\r\n", path, host);
      }
    } else { /* gopher */
      sprintf(buffer, "%s\r\n", path);
    }

    if (net_send(socket, buffer, strlen(buffer)) != (int)strlen(buffer)) {
      net_abort(socket);
      break;
    }

    /* eat all headers out if using http */
    if (proto == PARSEURL_PROTO_HTTP) {
      char httpokflag = 0;
      if (sock_getline(socket, buffer, buffer_max) > 0) {
        if (fdnpkg_strcasestr(buffer, "HTTP/1.0 200") == buffer) httpokflag = 1;
        if (fdnpkg_strcasestr(buffer, "HTTP/1.1 200") == buffer) httpokflag = 1;
      }
      if (httpokflag == 0) { /* something is not right */
        net_abort(socket);
        break;
      }
      /* eat all other headers, but look out for the content-length, if present */
      while (sock_getline(socket, buffer, buffer_max) > 0) {
        if (fdnpkg_strcasestr(buffer, "CONTENT-LENGTH:") == buffer) {
          /* get the value */
          expectedfilelen = atol(buffer + 15);
          if (expectedfilelen < 1) expectedfilelen = -1;
        }
      }
    }

    fd = fopen(outputfile, "wb");
    if (fd == NULL) { /* this should not fail */
      net_abort(socket);
      break;
    }

    for (;;) {
      byteread = net_recv(socket, buffer, buffer_max);
      curtime = time(NULL);
      if (((curtime - lastrefreshtime > 1) || (byteread < 0)) && (downloadingstring != NULL)) {
        lastrefreshtime = curtime;
        printf("\r"); /* go back to start of line, but don't scroll down */
        printf(downloadingstring, shorturlfilename, fdlen);
        if (expectedfilelen > 100) { /* only if > 100 because otherwise we'd get a nasty division by zero! */
          printf(" [%ld%%]", ((fdlen / 100) * 100) / (expectedfilelen / 100));
        }
      }
      if (byteread < 0) break;
      fdlen += byteread;
      fwrite(buffer, byteread, 1, fd);
    }
    if (downloadingstring != NULL) printf("\n");
    if (fdlen > 0) { /* close the connection nicely if we got at least 1 byte of data */
      net_close(socket);
    } else { /* otherwise abort asap, the remote server is either non existant or in really bad shape anyway */
      net_abort(socket);
    }
    fclose(fd);
    result = fdlen;
    break;
  }
  if (host != NULL) free(host);
  if (path != NULL) free(path);
  if (buffer != NULL) free(buffer);
  return(result);
}
