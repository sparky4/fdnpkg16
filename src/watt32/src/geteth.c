/*!\file geteth.c
 *
 *  `/etc/ethers' file functions for Watt-32.
 */

/*  Copyright (c) 1997-2002 Gisle Vanem <giva@bgnett.no>
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. All advertising materials mentioning features or use of this software
 *     must display the following acknowledgement:
 *       This product includes software developed by Gisle Vanem
 *       Bergen, Norway.
 *
 *  THIS SOFTWARE IS PROVIDED BY ME (Gisle Vanem) AND CONTRIBUTORS ``AS IS''
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL I OR CONTRIBUTORS BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *  28.apr 2000 - Created
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wattcp.h"
#include "misc.h"
#include "pcarp.h"
#include "pcconfig.h"
#include "udp_dom.h"
#include "strings.h"
#include "netaddr.h"
#include "get_xby.h"

#if defined(USE_BSD_API)

#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/route.h>

struct ethent {
       eth_address    eth_addr;  /* ether/MAC address of host */
       DWORD          ip_addr;   /* IP-address (host order) */
       char          *name;      /* host-name for IP-address */
       struct ethent *next;
     };

static struct ethent *eth0 = NULL;
static int    num_entries  = -1;
static char   ethersFname [MAX_PATHLEN] = "";

static int  getethent (const char *line, eth_address *e, char *name, size_t max);
static void endethent (void);

#if defined(TEST_PROG)
  #define TRACE(s)  printf s
#else
  #define TRACE(s)  ((void)0)
#endif

void InitEthersFile (const char *fname)
{
  if (fname && *fname)
     StrLcpy (ethersFname, fname, sizeof(ethersFname));
}

const char *GetEthersFile (void)
{
  return (ethersFname[0] ? ethersFname : NULL);
}

int NumEtherEntries (void)
{
  return (num_entries);
}

void ReadEthersFile (void)
{
  FILE *file;
  char  buf [2*MAX_HOSTLEN];

  if (!ethersFname[0])
     return;

  file = fopen (ethersFname, "rt");
  if (!file)
  {
    netdb_warn (ethersFname);
    return;
  }

  while (fgets(buf,sizeof(buf),file))
  {
    struct ethent  *e;
    struct hostent *h;
    char   hostname [MAX_HOSTLEN], *p;
    int    save;
    eth_address eth;

    p = strrtrim (strltrim(buf));
    if (memchr ("\r\n#;\0", *p, 5))
       continue;

    TRACE (("line '%s', ", p));

    if (!getethent(p,&eth,hostname,sizeof(hostname)))
       continue;

    if (num_entries == -1)
       num_entries = 0;
    num_entries++;
    save = called_from_resolve;
    called_from_resolve = TRUE;   /* prevent a DNS lookup */
    h = gethostbyname (hostname);
    called_from_resolve = save;

    if (!h)
    {
      TRACE (("gethostbyname() failed\n"));
      continue;
    }
    TRACE (("\n"));

    e = calloc (sizeof(*e), 1);
    if (!e)
       break;

    memcpy (&e->eth_addr, &eth, sizeof(e->eth_addr));
    e->ip_addr = ntohl (*(DWORD*)h->h_addr);
    _arp_add_cache (e->ip_addr, &e->eth_addr, FALSE);
    if (h->h_name)
    {
      e->name = strdup (h->h_name);
      if (!e->name)
         break;
    }
    e->next = eth0;
    eth0 = e;
  }

  fclose (file);
  RUNDOWN_ADD (endethent, 250);
}

/*
 * Parse a string of text containing an ethernet address and
 * hostname/IP-address and separate it into its component parts.
 */
static int getethent (const char *line, eth_address *e,
                      char *name, size_t name_max)
{
  const char *host_ip;
  size_t      len, i;
  WORD        eth[6];

  if (sscanf(line, "%hx:%hx:%hx:%hx:%hx:%hx",
             &eth[0], &eth[1], &eth[2],
             &eth[3], &eth[4], &eth[5]) != 6)
  {
    TRACE (("sscanf() failed\n"));
    return (0);
  }

  host_ip = strchr (line, ' ');
  if (!host_ip)
     return (0);

  len = strlen (host_ip);
  if (len < 1 || len >= name_max)
     return  (0);

  sscanf (host_ip, "%s", name);
  for (i = 0; i < sizeof(*e); i++)
      ((BYTE*)e)[i] = (BYTE) eth[i];
  return (1);
}

/*
 * Free memory in 'eth0' array
 */
static void endethent (void)
{
  struct ethent *e, *next;

  if (_watt_fatal_error)
     return;

  for (e = eth0; e; e = next)
  {
    free (e->name);
    next = e->next;
    free (e);
  }
  eth0 = NULL;
  num_entries = 0;
}
#endif  /* USE_BSD_API */

#if defined(TEST_PROG)

#include "pcdbug.h"
#include "sock_ini.h"

int main (void)
{
#if defined(USE_BSD_API)
  const struct ethent *e;

  dbug_init();
  sock_init();

  if (!ethersFname[0])
  {
    printf ("No ETHERS line found in WATTCP.CFG\n");
    return (-1);
  }

  printf ("\n%s entries:\n", ethersFname);
  for (e = eth0; e; e = e->next)
      printf ("%02X:%02X:%02X:%02X:%02X:%02X = %-15.15s (%s)\n",
              (int)e->eth_addr[0], (int)e->eth_addr[1],
              (int)e->eth_addr[2], (int)e->eth_addr[3],
              (int)e->eth_addr[4], (int)e->eth_addr[5],
              _inet_ntoa(NULL,e->ip_addr), e->name ? e->name : "none");
  fflush (stdout);
#else
  puts ("\"USE_BSD_API\" not defined");
#endif /* USE_BSD_API */
  return (0);
}
#endif /* TEST_PROG */

