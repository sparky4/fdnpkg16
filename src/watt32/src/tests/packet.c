#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <tcp.h>

#include <sys/socket.h>
#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_packe.h>

#define TRUE  1
#define FALSE 0

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

static void packet_recv (const char *what_sock, int s)
{
  struct sockaddr_ll addr;
  const struct ether_header eth;
  int   addr_len = sizeof(addr);
  int   len = recvfrom (s, (char*)&eth, sizeof(eth), 0,
                        (struct sockaddr*)&addr, &addr_len);

  printf ("%s: eth %04Xh: pkt-type %d, from %02X:%02X:%02X:%02X:%02X:%02X\n",
          what_sock, ntohs(addr.sll_protocol), addr.sll_pkttype,
          eth.ether_shost[0], eth.ether_shost[1],
          eth.ether_shost[2], eth.ether_shost[3],
          eth.ether_shost[4], eth.ether_shost[5]);
}

int main (int argc, char **argv)
{
  BOOL quit = FALSE;
  int  s1, s2;

  printf ("Linux style AF_PACKET example. Press ESC to quit\n");

  if (argc > 1 && !strcmp(argv[1],"-d"))
     dbug_init();

  s1 = socket (AF_PACKET, SOCK_PACKET, 0);
  if (s1 < 0)
  {
    perror ("socket");
    return (-1);
  }
  s2 = socket (AF_PACKET, SOCK_PACKET, 0);
  if (s2 < 0)
  {
    perror ("socket");
    close_s (s1);
    return (-1);
  }

  while (!quit)
  {
    struct timeval tv = { 1, 0 };
    fd_set rd;
    int    num;

    FD_ZERO (&rd);
    FD_SET (s1, &rd);
    FD_SET (s2, &rd);
    FD_SET (STDIN_FILENO, &rd);
    num = max (s1,s2) + 1;

    if (select_s (num, &rd, NULL, NULL, &tv) < 0)
    {
      perror ("select_s");
      quit = TRUE;
    }

    if (FD_ISSET(STDIN_FILENO,&rd) && getche() == 27)
       quit = TRUE;

    if (FD_ISSET(s1,&rd))
       packet_recv ("sock-1", s1);

    if (FD_ISSET(s2,&rd))
       packet_recv ("sock-2", s2);
  }

  close_s (s1);
  close_s (s2);
  return (0);
}

