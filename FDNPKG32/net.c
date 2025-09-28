/*
 *  This file is part of the FDNPKG project.
 *  It provides a set of basic network-related functions.
 *
 *  Copyright (C) 2013-2016 Mateusz Viste
 *
 * Provides all network functions used by FDNPKG, wrapped around the WatTCP TCP/IP stack.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* WatTCP includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <tcp.h>

#include "version.h"
#include "net.h" /* include self for control */


#define WINDOWSIZE  (16*1024)
#define BUFFERSIZE  2048

#define debugMode 0
#define verboseMode 0




/* this is a wrapper around the wattcp lookup_host(), but with a small integrated cache.
   returns 0 if resolution fails. */
unsigned long net_dnsresolve(const char *name) {
  unsigned long hostaddr = 0;
  static unsigned long cacheaddr[16];
  static char cachename[16][64] = {{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}};
  int x, freeentry = -1;
  int namelen = strlen(name);
  if (namelen < 64) {
    for (x = 0 ; x < 16 ; x++) {
      if (cachename[x][0] == 0) { /* empty (free) entry */
        if (freeentry == -1) freeentry = x; /* remember it for later */
      } else { /* else check if it's what we need */
        if (strcmp(cachename[x], name) == 0) return(cacheaddr[x]); /* if found in cache, stop here */
      }
    }
  }
  hostaddr = lookup_host(name,NULL);
  if (hostaddr == 0) return(0); /* dns resolving error */
  if ((namelen < 64) && (freeentry >= 0)) { /* if not longer than maxlen, and cache not full, then save it */
    strcpy(cachename[freeentry], name); /* save name in cache */
    cacheaddr[freeentry] = hostaddr; /* save addr in cache */
  }
  return(hostaddr);
}



static int dummy_printf(const char * format, ...) {
  if (format == NULL) return(-1);
  return(0);
}

/* must be called before using libtcp. returns 0 on success, or non-zero if network subsystem is not available. */
int net_init() {
  tzset();
  _printf = dummy_printf;  /* this is to avoid watt32 printing its stuff to console */
  return(sock_init());
}


struct net_tcpsocket *net_connect(unsigned long ipaddr, int port) {
  struct net_tcpsocket *resultsock;
  int status = 0;
  int *statusptr = &status;
  resultsock = malloc(sizeof(*resultsock));
  resultsock->buffersize = BUFFERSIZE;
  resultsock->buffer = malloc(resultsock->buffersize);
  resultsock->sock   = malloc(sizeof(tcp_Socket));

  if (!tcp_open(resultsock->sock, 0, ipaddr, port, NULL)) {
    free(resultsock->buffer);
    free(resultsock->sock);
    free(resultsock);
    return(NULL);
  }
  sock_setbuf (resultsock->sock, resultsock->buffer, resultsock->buffersize);
  sock_wait_established (resultsock->sock, sock_delay, NULL, &status);
  sock_tick (resultsock->sock, statusptr);      /* in case they sent reset */
  return(resultsock);

 sock_err:
  free(resultsock->buffer);
  free(resultsock->sock);
  free(resultsock);
  return(NULL);
}


/* Sends data on socket 'socket'.
   Returns the number of bytes sent on success, and <0 otherwise. The error code can be translated into a human error message via libtcp_strerr(). */
int net_send(struct net_tcpsocket *socket, char *line, int len) {
  int res;
  int status = 0;
  int *statusptr = &status;
  res = sock_write(socket->sock, line, len);
  sock_tick (socket->sock, statusptr);   /* call this to let WatTCP hanle its internal stuff */
  return(res);
 sock_err:
  return(-1);
}


/* Reads data from socket 'sock' and write it into buffer 'buff', until end of connection or until buff is filled with maxlen bytes. Returns the amount of data read (in bytes) on success, or a negative value otherwise. Returns 0 if there is no data awaiting on the socket. */
int net_recv(struct net_tcpsocket *socket, char *buff, int maxlen) {
  int i;
  int status = 0;
  int *statusptr = &status;
  sock_tick (socket->sock, statusptr);  /* call this to let WatTCP hanle its internal stuff */
  i = sock_fastread(socket->sock, buff, maxlen);
  return(i);
 sock_err:
  return(-1);
}


/* Close the 'sock' socket. */
void net_close(struct net_tcpsocket *socket) {
  int status = 0;
  sock_close(socket->sock);
  sock_wait_closed(socket->sock, sock_delay, NULL, &status);
 sock_err:
  free(socket);
  return;
}


/* Close the 'sock' socket immediately (to be used when the peer is behaving wrongly) - this is much faster than net_close(). */
void net_abort(struct net_tcpsocket *socket) {
  sock_abort(socket->sock);
  free(socket);
  return;
}
