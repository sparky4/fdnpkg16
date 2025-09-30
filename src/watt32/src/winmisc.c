/*!\file winmisc.c
 *
 *  Various stuff for Windows only.
 *
 *  Copyright (c) 2004 Gisle Vanem <giva@bgnett.no>
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
 */

#include "socket.h"
#include "udp_dom.h"
#include "get_xby.h"
#include "sock_ini.h"

#if defined(WIN32) && defined(HAVE_WINDNS_H)

#include <winsock.h>
#include <windns.h>

CRITICAL_SECTION _watt_crit_sect;
BOOL             _watt_is_win9x = FALSE;

static BOOL is_gui_app = FALSE;

/*
 * 'dns_windns' bits.
 */
#define WINDNS_QUERY_A4     0x0001
#define WINDNS_QUERY_A6     0x0002
#define WINDNS_QUERY_PTR4   0x0004
#define WINDNS_QUERY_PTR6   0x0008
#define WINDNS_CACHEPUT_A4  0x0100
#define WINDNS_CACHEPUT_A6  0x0200

#ifndef DNS_TYPE_AAAA
#define DNS_TYPE_AAAA  0x001C
#endif

#ifndef DNS_ERROR_RESPONSE_CODES_BASE
#define DNS_ERROR_RESPONSE_CODES_BASE  9000
#endif

#ifndef DNS_ERROR_RCODE_LAST
#define DNS_ERROR_RCODE_LAST  9018
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

/*
 * DnsApi.dll is used dynamically. So we need
 * these function pointers.
 */
typedef DNS_STATUS (WINAPI *DnsQuery_A_func) (
        IN     const char  *name,
        IN     WORD         wType,
        IN     DWORD        options,
        IN     IP4_ARRAY   *aipServers,
        IN OUT DNS_RECORD **queryResults,
        IN OUT void       **reserved);

typedef DNS_STATUS (WINAPI *DnsModifyRecordsInSet_A_func) (
        IN     DNS_RECORD *addRecords,
        IN     DNS_RECORD *deleteRecords,
        IN     DWORD       options,
        IN     HANDLE      context,    OPTIONAL
        IN     IP4_ARRAY  *serverList, OPTIONAL
        IN     void       *reserved);

typedef void (WINAPI *DnsFree_func) (
        IN OUT void *data,
        IN     DNS_FREE_TYPE freeType);

static DnsFree_func                 _DnsFree                 = NULL;
static DnsQuery_A_func              _DnsQuery_A              = NULL;
static DnsModifyRecordsInSet_A_func _DnsModifyRecordsInSet_A = NULL;

static HINSTANCE windns_mod = NULL;

/*
 * Various stuff to initialise.
 */
static BOOL get_win_version (WORD *ver, BOOL *is_win9x)
{
  OSVERSIONINFO ovi;
  DWORD os_ver = GetVersion();
  DWORD major_ver = LOBYTE (LOWORD(os_ver));

  *is_win9x = (os_ver >= 0x80000000 && major_ver >= 4);

  memset (&ovi, 0, sizeof(ovi));
  ovi.dwOSVersionInfoSize = sizeof(ovi);
  if (!GetVersionEx(&ovi))
     return (FALSE);

  *ver = (WORD)(ovi.dwMajorVersion << 8) +
         (WORD)ovi.dwMinorVersion;
  return (TRUE);
}

/*
 * Check if program is a GUI app.
 */
static BOOL check_gui_app (void)
{
  const IMAGE_DOS_HEADER *dos;
  const IMAGE_NT_HEADERS *nt;
  HMODULE mod = GetModuleHandle (NULL);
  HANDLE  hnd = GetStdHandle (STD_OUTPUT_HANDLE);

  WIN_ASSERT (mod != NULL);

  if (hnd && GetFileType(hnd) != FILE_TYPE_UNKNOWN)
     return (FALSE);

  dos = (const IMAGE_DOS_HEADER*) mod;
  nt  = (const IMAGE_NT_HEADERS*) ((const BYTE*)mod + dos->e_lfanew);
  return (nt->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);
}

static int gui_printf (const char *fmt, ...)
{
  char    buf[1024];
  int     len;
  char   *prog_name, *s;
  DWORD   mb_flags;
  va_list args;

  va_start (args, fmt);
  len = _vsnprintf (buf, sizeof(buf), fmt, args);

  s = (char*) get_argv0();
  if ((prog_name = strrchr(s,'\\')) == NULL &&
      (prog_name = strrchr(s, ':')) == NULL)
       prog_name = s;
  else prog_name++;

  mb_flags = MB_ICONSTOP | MB_SETFOREGROUND;
  mb_flags |= _watt_is_win9x ? MB_SYSTEMMODAL : MB_TASKMODAL;
  MessageBox (NULL, buf, prog_name, mb_flags);

  va_end (args);
  return (len);
}

static void win32_exit (void)
{
  if (windns_mod)
     FreeLibrary (windns_mod);
  windns_mod = NULL;

#if defined(__LCC__)
  DeleteCriticalSection ((struct _CRITICAL_SECTION*)&_watt_crit_sect);
#else
  DeleteCriticalSection (&_watt_crit_sect);
#endif
}

/*
 * Return err-number+string for 'err'. Use only with GetLastError().
 * Does not handle libc errno's. Remove trailing [\r\n.]
 */
char *win_strerror (DWORD err)
{
  static char buf[512+20];
  char   err_buf[512], *p;

  if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
                      LANG_NEUTRAL, err_buf, sizeof(err_buf)-1, NULL))
     strcpy (err_buf, "Unknown error");
  _snprintf (buf, sizeof(buf), "%lu/0x%lX %s", err, err, err_buf);
  rip (buf);
  p = strrchr (buf, '.');
  if (p && p[1] == '\0')
     *p = '\0';
  return (buf);
}

/*
 * WIN_ASSERT() fail handler.
 */
void assert_fail_win (const char *file, unsigned line, const char *what)
{
  _snprintf (_watt_assert_buf, sizeof(_watt_assert_buf),
             "%s (%u): Assertion `%s' failed. GetLastError() %s\n\n",
             file, line, what, win_strerror(GetLastError()));

  is_gui_app ? gui_printf(_watt_assert_buf) : (*_printf)("%s\n", _watt_assert_buf);
  rundown_run();
  exit (-1);
}

/*
 * Called from init_misc() to initialise Win32 specific things.
 */
BOOL init_winmisc (void)
{
#if defined(__LCC__)
  InitializeCriticalSection ((struct _CRITICAL_SECTION*)&_watt_crit_sect);
#else
  InitializeCriticalSection (&_watt_crit_sect);
#endif

  _watt_os_ver = 0x0400;   /* defaults to Win-NT 4.0 */
  if (!get_win_version (&_watt_os_ver, &_watt_is_win9x))
     return (FALSE);

  windns_mod = LoadLibrary ("dnsapi.dll");
  if (windns_mod)
  {
    _DnsQuery_A = (DnsQuery_A_func)
      GetProcAddress (windns_mod, "DnsQuery_A");

    _DnsFree = (DnsFree_func)
      GetProcAddress (windns_mod, "DnsFree");

    _DnsModifyRecordsInSet_A = (DnsModifyRecordsInSet_A_func)
      GetProcAddress (windns_mod, "DnsModifyRecordsInSet_A");
  }

  is_gui_app = check_gui_app();

  RUNDOWN_ADD (win32_exit, 310);
  return (TRUE);
}

/*
 * Perform a query on the WinDns cache. We ask only for
 * an address (A/AAAA records) or a name (PTR record).
 */
static BOOL WinDnsQueryCommon (WORD type, const void *what,
                               void *result, size_t size)
{
  DNS_RECORD *rr, *dr = NULL;
  DNS_STATUS  rc;
  BOOL   found;
  DWORD  opt = DNS_QUERY_NO_WIRE_QUERY | /* query the cache only */
               DNS_QUERY_NO_NETBT |      /* no NetBT names */
               DNS_QUERY_NO_HOSTS_FILE;  /* no Winsock hosts file */

  if (!_DnsQuery_A || !_DnsFree)
     return (FALSE);

  rc = (*_DnsQuery_A) ((const char*)what, type, opt, NULL, &dr, NULL);

  TCP_CONSOLE_MSG (2, ("DnsQuery_A: type %d, dr %p: %s\n",
                   type, dr, win_strerror(rc)));

  if (rc != ERROR_SUCCESS || !dr)
     return (FALSE);

  dom_ttl = dr->dwTtl;
  found = FALSE;

  for (rr = dr; rr; rr = rr->pNext)
  {
    TCP_CONSOLE_MSG (2, ("RR-type: %d: ", rr->wType));

    /* Use only 1st A/AAAA record
     */
    if (rr->wType == DNS_TYPE_A && type == DNS_TYPE_A)
    {
      DWORD ip = ntohl (rr->Data.A.IpAddress);

      TCP_CONSOLE_MSG (2, ("A: %s, ttl %lus\n",
                       _inet_ntoa(NULL,ip), rr->dwTtl));
      if (!found)
         *(DWORD*) result = ip;
      found = TRUE;
    }

#if defined(USE_BSD_API) || defined(USE_IPV6)
    else if (rr->wType == DNS_TYPE_AAAA && type == DNS_TYPE_AAAA)
    {
      const void *ip6 = &rr->Data.AAAA.Ip6Address.IP6Dword[0];

      TCP_CONSOLE_MSG (2, ("AAAA: %s\n", _inet6_ntoa(ip6)));
      if (!found)
         memcpy (result, ip6, size);
      found = TRUE;
    }
#endif

    else if (rr->wType == DNS_TYPE_PTR && type == DNS_TYPE_PTR)
    {
      StrLcpy (result, dr->Data.PTR.pNameHost, size);
      TCP_CONSOLE_MSG (2, ("PTR: %s\n", (const char*)result));
    }
    else if (rr->wType == DNS_TYPE_CNAME)
    {
      StrLcpy (dom_cname, dr->Data.CNAME.pNameHost, sizeof(dom_cname));
      TCP_CONSOLE_MSG (2, ("CNAME: %s\n", dom_cname));
    }
    else
      TCP_CONSOLE_MSG (2, ("\n"));
  }
  (*_DnsFree) (dr, DnsFreeRecordList);
  return (TRUE);
}

BOOL WinDnsQuery_A4 (const char *name, DWORD *ip)
{
  if (!(dns_windns & WINDNS_QUERY_A4))
     return (FALSE);
  return WinDnsQueryCommon (DNS_TYPE_A, name, ip, sizeof(*ip));
}

BOOL WinDnsQuery_A6 (const char *name, void *ip)
{
  if (!(dns_windns & WINDNS_QUERY_A6))
     return (FALSE);
  return WinDnsQueryCommon (DNS_TYPE_AAAA, name, ip, sizeof(ip6_address));
}

BOOL WinDnsQuery_PTR4 (DWORD ip, char *name, size_t size)
{
  if (!(dns_windns & WINDNS_QUERY_PTR4))
     return (FALSE);
  return WinDnsQueryCommon (DNS_TYPE_PTR, &ip, name, size);
}

BOOL WinDnsQuery_PTR6 (const void *ip, char *name, size_t size)
{
  if (!(dns_windns & WINDNS_QUERY_PTR6))
     return (FALSE);
  return WinDnsQueryCommon (DNS_TYPE_PTR, &ip, name, size);
}

/*
 * This doesn't seem to simply put a name/IP pair in the
 * local cache, but do a complete registration with the
 * Winsock registered DNS serve(s). Hence off by default.
 */
BOOL WinDnsCachePut_A4 (const char *name, DWORD ip4)
{
  DNS_RECORD rr;
  DNS_STATUS rc;
  DWORD      opt = DNS_UPDATE_SECURITY_OFF |
                   DNS_UPDATE_CACHE_SECURITY_CONTEXT;

  if (!_DnsModifyRecordsInSet_A ||
      !(dns_windns & WINDNS_CACHEPUT_A4))
     return (FALSE);

  memset (&rr, 0, sizeof(rr));

  rr.pName = strdup (name);
  rr.wType = DNS_TYPE_A;
  rr.dwTtl = netdbCacheLife;
  rr.Data.A.IpAddress = htonl (ip4);
  rr.wDataLength      = sizeof(rr.Data.A);

  rc = (*_DnsModifyRecordsInSet_A) (&rr, NULL, opt,
                                    NULL, NULL, NULL);
  if (rr.pName)
     free (rr.pName);

  TCP_CONSOLE_MSG (2, ("DnsModifyRecordsInSet_A: %s ",
                   win_strerror(rc)));

  if (rc >= DNS_ERROR_RESPONSE_CODES_BASE && rc <= DNS_ERROR_RCODE_LAST)
     dns_windns &= ~WINDNS_CACHEPUT_A4;  /* don't do this again */
  return (rc == ERROR_SUCCESS);
}

BOOL WinDnsCachePut_A6 (const char *name, const void *ip6)
{
  if (!(dns_windns & WINDNS_CACHEPUT_A6))
     return (FALSE);
  ARGSUSED (name);
  ARGSUSED (ip6);
  return (FALSE);
}

int __stdcall WSAStartup (WORD version_required, WSADATA *wsa_data)
{
  int rc;

  if (version_required > MAKEWORD(2,2))
     return (EINVAL);  /* we don't have WSAVERNOTSUPPORTED */

  _watt_do_exit = 0;
  rc = watt_sock_init (0, 0);

  wsa_data->wVersion     = version_required;
  wsa_data->wHighVersion = 2;
  wsa_data->iMaxSockets  = MAX_SOCKETS;
  wsa_data->iMaxUdpDg    = MAX_SOCKETS;
  wsa_data->lpVendorInfo = NULL;
  wsa_data->szSystemStatus[0] = '\0';
  strcpy (wsa_data->szDescription, "Watt-32 tcp/ip");
  return (rc);
}

int __stdcall WSACleanup (void)
{
  sock_exit();
  return (0);
}

int __stdcall __WSAFDIsSet (int s, winsock_fd_set *fd)
{
  UNFINISHED();
  ARGSUSED (s);
  ARGSUSED (fd);
  return (0);
}
#endif  /* WIN32 && HAVE_WINDNS_H */

