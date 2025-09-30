/*
 * Copyright (c) 1999 - 2003
 * Politecnico di Torino.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the Politecnico
 * di Torino, and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

 /*
  * Changes for Watt-32:
  *  - Merged Packet32.c and AdInfo.c into one.
  *  - Removed all WanPacket and Dag API support.
  *  - Rewritten for ASCII function (no Unicode).
  *  - Lots of simplifications.
  */
#include <stdio.h>
#include <stdarg.h>
#include <io.h>

#if defined(WIN32) || defined(_WIN32)

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>       /* GlobalAllocPtr() */
#include <winsvc.h>         /* SERVICE_ALL_ACCESS... */
#include <arpa/inet.h>      /* inet_aton() */
#include <sys/socket.h>     /* AF_INET etc. */
#include <process.h>

#include "wattcp.h"
#include "misc.h"
#include "timer.h"
#include "strings.h"
#include "pcconfig.h"
#include "pcdbug.h"
#include "ntddndis.h"
#include "cpumodel.h"
#include "packet32.h"

#if defined(USE_DYN_PACKET)
struct dyn_table dyn_funcs;
static HINSTANCE packet_mod = NULL;

static BOOL LoadPacketFunctions (void);

#else

/**
 * Current NPF.SYS version.
 */
static char npf_driver_version[64] = "Unknown npf.sys version";

/**
 * Name constants.
 */
static const char NPF_service_name[]      = "NPF";
static const char NPF_service_descr[]     = "Netgroup Packet Filter";
static const char NPF_registry_location[] = "SYSTEM\\CurrentControlSet\\Services\\NPF";
static const char NPF_driver_path[]       = "system32\\drivers\\npf.sys";

/**
 * Head of the adapter information list. This list is populated when
 * packet.dll is linked by the application.
 */
static ADAPTER_INFO *adapters_list = NULL;

/**
 * Mutex that protects the adapter information list.
 * \note every API that takes an ADAPTER_INFO as parameter assumes
 *       that it has been called with the mutex acquired.
 */
static HANDLE adapters_mutex = INVALID_HANDLE_VALUE;

static BOOL PopulateAdaptersInfoList (void);
static BOOL FreeAdaptersInfoList (void);
static BOOL GetFileVersion (const char *file_name,
                            char *version_buf,
                            size_t version_buf_len);
#endif

/*
 * Lack of a C99 compiler makes this a bit harder to trace.
 */
static const char *trace_func;

#if !defined(USE_DEBUG)
  #define WINPCAP_TRACE(args)  ((void)0)
#else
  #define WINPCAP_TRACE(args) do { \
          trace_line = __LINE__;   \
          winpcap_trace args ;     \
        } while (0)

  static FILE *trace_file = NULL;
  static UINT  trace_line;
  static DWORD start_tick;

  /**
   * Dumps a registry key to disk in text format. Uses regedit.
   *
   * \param key_name Name of the key to dump. All its subkeys will be
   *        saved recursively.
   * \param file_name Name of the file that will contain the dump.
   * \return If the function succeeds, the return value is nonzero.
   *
   * For debugging purposes, we use this function to obtain some registry
   * keys from the user's machine.
   */
  static void PacketDumpRegistryKey (const char *key_name, const char *file_name)
  {
    char command[256];

    /* Let regedit do the dirty work for us
     */
    _snprintf (command, sizeof(command), "regedit /e %s %s", file_name, key_name);
    system (command);
  }

  static void winpcap_trace (const char *fmt, ...)
  {
    static BOOL init = FALSE;
    va_list args;

    if (!trace_file)
       return;

    if (!init)
       fputs ("dT [ms] line\n------------------------------------------"
              "------------------------------------------\n", trace_file);
    init = TRUE;

    fprintf (trace_file, "%5lu, %4u %s(): ", GetTickCount() - start_tick,
             trace_line, trace_func);
    va_start (args, fmt);
    vfprintf (trace_file, fmt, args);
    fflush (trace_file);
    if (ferror(trace_file))
    {
      TCP_CONSOLE_MSG (1, ("error writing WinPcap dump file; %s\n",
                       strerror(errno)));
      fclose (trace_file);
      trace_file = NULL;
    }
    va_end (args);
  }
#endif

/**
 * The winpcap init/deinit function (formerly DllMain).
 */
BOOL PacketInitModule (BOOL init, FILE *dump)
{
  BOOL rc;

  trace_func = "PacketInitModule";

  if (!init)
  {
    WINPCAP_TRACE (("deinit\n"));

#if defined(USE_DYN_PACKET)
    if (packet_mod)
       FreeLibrary (packet_mod);
    packet_mod = NULL;
    rc = TRUE;
#else
    FreeAdaptersInfoList();
    rc = CloseHandle (adapters_mutex);
    adapters_mutex = INVALID_HANDLE_VALUE;
#endif
    return (rc);
  }

#if defined(USE_DEBUG)
  trace_file = dump;
  start_tick = GetTickCount();
  WINPCAP_TRACE (("init\n"));

  if (debug_on >= 5 && !_watt_is_win9x)
  {
    /* dump a bunch of registry keys
     */
    PacketDumpRegistryKey (
      "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet"
      "\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}",
      "adapters.reg");
    PacketDumpRegistryKey ("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet"
                           "\\Services\\Tcpip", "tcpip.reg");
    PacketDumpRegistryKey ("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet"
                           "\\Services\\NPF", "npf.reg");
    PacketDumpRegistryKey ("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet"
                           "\\Services", "services.reg");
  }
#endif

#if defined(USE_DYN_PACKET)
  rc = LoadPacketFunctions();

#else
  /* Create the mutex that will protect the adapter information list
   */
  adapters_mutex = CreateMutex (NULL, FALSE, NULL);

  /* Retrieve NPF.sys version information from the file
   */
  GetFileVersion ("drivers\\npf.sys", npf_driver_version,
                  sizeof(npf_driver_version));

  /* Populate the 'adapters_list' list.
   */
  rc = PopulateAdaptersInfoList();

#if defined(USE_DEBUG)
  if (trace_file)
  {
    const struct ADAPTER_INFO *ai;

    fputs ("\nKnown adapters:\n", trace_file);
    for (ai = adapters_list; ai; ai = ai->Next)
        fprintf (trace_file, "  %s, `%s'\n", ai->Name, ai->Description);
    fputs ("\n", trace_file);
  }
#endif
#endif

  return (rc);
}

#if defined(USE_DYN_PACKET)

typedef void (__cdecl *packet_func) (void);

static BOOL GetPacketFunc (packet_func *func, const char *name)
{
  *func	= (packet_func) GetProcAddress (packet_mod, name);
  trace_func = "GetPacketFunc";
  WINPCAP_TRACE (("  %s -> 0x%p\n", name, *func));
  if (!*func)
     return (FALSE);
  return (TRUE);
}

static BOOL LoadPacketFunctions (void)
{
  packet_mod = LoadLibrary ("packet.dll");
  if (!packet_mod)
  {
    trace_func = "LoadPacketFunctions";
    WINPCAP_TRACE (("Cannot load packet.dll; %s\n", win_strerror(GetLastError())));
    return (FALSE);
  }
  if (!GetPacketFunc((packet_func*)&dyn_funcs.PacketOpenAdapter, "PacketOpenAdapter") ||
      !GetPacketFunc((packet_func*)&dyn_funcs.PacketCloseAdapter, "PacketCloseAdapter") ||
      !GetPacketFunc((packet_func*)&dyn_funcs.PacketRequest, "PacketRequest") ||
      !GetPacketFunc((packet_func*)&dyn_funcs.PacketGetDriverVersion, "PacketGetDriverVersion") ||
      !GetPacketFunc((packet_func*)&dyn_funcs.PacketSetMode, "PacketSetMode") ||
      !GetPacketFunc((packet_func*)&dyn_funcs.PacketSetBuff, "PacketSetBuff") ||
      !GetPacketFunc((packet_func*)&dyn_funcs.PacketSetMinToCopy, "PacketSetMinToCopy") ||
      !GetPacketFunc((packet_func*)&dyn_funcs.PacketSetReadTimeout, "PacketSetReadTimeout") ||
      !GetPacketFunc((packet_func*)&dyn_funcs.PacketGetStatsEx, "PacketGetStatsEx") ||
      !GetPacketFunc((packet_func*)&dyn_funcs.PacketReceivePacket, "PacketReceivePacket") ||
      !GetPacketFunc((packet_func*)&dyn_funcs.PacketSendPacket, "PacketSendPacket"))
     return (FALSE);
  return (TRUE);
}

#else    /* rest of file */

/**
 * Sets the maximum possible lookahead buffer for the driver's
 * Packet_tap() function.
 *
 * \param AdapterObject Handle to the service control manager.
 * \return If the function succeeds, the return value is nonzero.
 *
 * The lookahead buffer is the portion of packet that Packet_tap() can
 * access from the NIC driver's memory without performing a copy.
 * This function tries to increase the size of that buffer.
 */
static BOOL PacketSetMaxLookaheadsize (const ADAPTER *AdapterObject)
{
  struct {
    PACKET_OID_DATA oidData;
    DWORD           filler;
  } oid;
  BOOL rc;

  trace_func = "PacketSetMaxLookaheadsize";

  /* Set the size of the lookahead buffer to the maximum available
   * by the the NIC driver.
   */
  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = OID_GEN_MAXIMUM_LOOKAHEAD;
  oid.oidData.Length = sizeof(oid.filler);
  rc = PacketRequest (AdapterObject, FALSE, &oid.oidData);

  trace_func = "PacketSetMaxLookaheadsize";
  WINPCAP_TRACE (("lookahead %lu, rc %d; %s\n",
                  *(DWORD*)&oid.oidData, rc,
                  !rc ? win_strerror(GetLastError()) : "okay"));

  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = OID_GEN_CURRENT_LOOKAHEAD;
  oid.oidData.Length = sizeof(oid.filler);
  rc = PacketRequest (AdapterObject, TRUE, &oid.oidData);

  trace_func = "PacketSetMaxLookaheadsize";
  WINPCAP_TRACE (("rc %d; %s\n",
                  rc, !rc ? win_strerror(GetLastError()) : "okay"));
  return (rc);
}

/**
 * Retrieves the event associated in the driver with a capture instance
 * and stores it in an ADAPTER structure.
 *
 * \param AdapterObject Handle to the service control manager.
 * \return If the function succeeds, the return value is nonzero.
 *
 * This function is used by PacketOpenAdapter() to retrieve the read event
 * from the driver by means of an ioctl call and set it in the ADAPTER
 * structure pointed by AdapterObject.
 */
static BOOL PacketSetReadEvt (ADAPTER *AdapterObject)
{
  DWORD BytesReturned;
  char  EventName[40];
  WCHAR EventNameU[20];

  trace_func = "PacketSetReadEvt";

  /* this tells the terminal service to retrieve the event from the global
   * namespace
   */
  strcpy (EventName, "Global\\");

  /* retrieve the Unicode name of the shared event from the driver
   */
  memset (&EventName, 0, sizeof(EventName));
  if (!DeviceIoControl(AdapterObject->hFile, pBIOCEVNAME, NULL, 0,
                       EventNameU, sizeof(EventNameU), &BytesReturned, NULL))
  {
    WINPCAP_TRACE (("error retrieving the event-name from the kernel\n"));
    return (FALSE);
  }

  _snprintf (EventName, sizeof(EventName), "Global\\%.13S", EventNameU);
  WINPCAP_TRACE (("  event-name %s\n", EventName));

  /* open the shared event
   */
  AdapterObject->ReadEvent = CreateEvent (NULL, TRUE, FALSE, EventName);

  /* On Win-NT4 "Global\" is not automatically ignored: try to use
   * simply the event name.
   */
  if (GetLastError() != ERROR_ALREADY_EXISTS)
  {
    if (AdapterObject->ReadEvent != INVALID_HANDLE_VALUE)
       CloseHandle (AdapterObject->ReadEvent);

    /* open the shared event */
    AdapterObject->ReadEvent = CreateEvent (NULL, TRUE, FALSE, EventName + 7);
  }

  if (AdapterObject->ReadEvent == INVALID_HANDLE_VALUE ||
      GetLastError() != ERROR_ALREADY_EXISTS)
  {
    WINPCAP_TRACE (("error retrieving the event from the kernel\n"));
    return (FALSE);
  }

  AdapterObject->ReadTimeOut = 0;  /* block until something received */
  return (TRUE);
}

/**
 * Installs the NPF device driver.
 *
 * \param ascmHandle Handle to the service control manager.
 * \param srvHandle  A pointer to a handle that will receive the
 *        pointer to the driver's service.
 * \return If the function succeeds, the return value is nonzero.
 *
 * This function installs the driver's service in the system using the
 * CreateService function.
 */
BOOL PacketInstallDriver (SC_HANDLE ascmHandle, SC_HANDLE *srvHandle)
{
  BOOL  result = FALSE;
  DWORD error  = 0UL;

  trace_func = "PacketInstallDriver";
  WINPCAP_TRACE (("\n"));

  *srvHandle = CreateService (ascmHandle,
                              NPF_service_name,
                              NPF_service_descr,
                              SERVICE_ALL_ACCESS,
                              SERVICE_KERNEL_DRIVER,
                              SERVICE_DEMAND_START,
                              SERVICE_ERROR_NORMAL,
                              NPF_driver_path, NULL, NULL,
                              NULL, NULL, NULL);
  if (!*srvHandle)
  {
    if (GetLastError() == ERROR_SERVICE_EXISTS)
    {
      /* npf.sys already exists */
      result = TRUE;
    }
  }
  else
  {
    /* Created service for npf.sys */
    result = TRUE;
  }

  if (result == TRUE && *srvHandle)
     CloseServiceHandle (*srvHandle);

  if (result == FALSE)
  {
    error = GetLastError();
    if (error != ERROR_FILE_NOT_FOUND)
       WINPCAP_TRACE (("failed; %s\n", win_strerror(error)));
  }

  SetLastError (error);
  return (result);
}

/**
 * Returns the version of a .dll or .exe file
 */
static BOOL GetFileVersion (const char *file_name,
                            char       *version_buf,
                            size_t      version_buf_len)
{
  DWORD     ver_info_size;         /* Size of version information block */
  DWORD     err, ver_hnd = 0;      /* An 'ignored' parameter, always '0' */
  UINT      bytes_read;
  char      sub_block[64];
  void     *res_buf, *vff_info = NULL;
  HINSTANCE mod;
  BOOL      rc = FALSE;

  DWORD (WINAPI *_GetFileVersionInfoSize) (char *, DWORD*);
  BOOL  (WINAPI *_GetFileVersionInfo) (char *, DWORD, DWORD, void *);
  BOOL  (WINAPI *_VerQueryValue) (const void **, char *, void **, UINT *);

  const struct LANG_AND_CODEPAGE {
               WORD language;
               WORD code_page;
            } *lang_info;
  void *lang_info_ptr;

  trace_func = "GetFileVersion";

  mod = LoadLibrary ("version.dll");
  if (!mod)
  {
    WINPCAP_TRACE (("Didn't find version.dll\n"));
    return (FALSE);
  }

  _GetFileVersionInfoSize = (DWORD (WINAPI*)(char*,DWORD*))
                              GetProcAddress (mod, "GetFileVersionInfoSizeA");
  _GetFileVersionInfo = (BOOL (WINAPI*)(char*,DWORD,DWORD,void*))
                          GetProcAddress (mod, "GetFileVersionInfoA");
  _VerQueryValue = (BOOL (WINAPI*)(const void**, char*,void**,UINT*))
                     GetProcAddress (mod, "VerQueryValueA");

  if (!_GetFileVersionInfoSize || !_GetFileVersionInfo || !_VerQueryValue)
  {
    WINPCAP_TRACE (("failed to load functons from version.dll\n"));
    goto quit;
  }

  /* Pull out the version information
   */
  ver_info_size = (*_GetFileVersionInfoSize) ((char*)file_name, &ver_hnd);
  WINPCAP_TRACE (("file %s, ver-size %lu\n", file_name, ver_info_size));

  if (!ver_info_size)
  {
    err = GetLastError();
    WINPCAP_TRACE (("failed to call GetFileVersionInfoSize; %s\n",
                    win_strerror(err)));
    goto quit;
  }

  vff_info = malloc (ver_info_size);
  if (!vff_info)
  {
    WINPCAP_TRACE (("failed to allocate memory\n"));
    goto quit;
  }

  if (!(*_GetFileVersionInfo) ((char*)file_name, ver_hnd,
                               ver_info_size, vff_info))
  {
    WINPCAP_TRACE (("failed to call GetFileVersionInfo\n"));
    goto quit;
  }

  /* Read the list of languages and code pages.
   */
  if (!(*_VerQueryValue) (vff_info, "\\VarFileInfo\\Translation",
                          &lang_info_ptr, &bytes_read) ||
      bytes_read < sizeof(*lang_info))
  {
    WINPCAP_TRACE (("failed to call VerQueryValue\n"));
    goto quit;
  }
  lang_info = (const struct LANG_AND_CODEPAGE*) lang_info_ptr;

  /* Create the file version string for the first (i.e. the only
   * one) language.
   */
  sprintf (sub_block, "\\StringFileInfo\\%04x%04x\\FileVersion",
           lang_info->language, lang_info->code_page);

  /* Retrieve the file version string for the language. 'res_buf' will
   * point into 'vff_info'. Hence it doesn't have to b freed.
   */
  if (!(*_VerQueryValue) (vff_info, sub_block, &res_buf, &bytes_read))
  {
    WINPCAP_TRACE (("failed to call VerQueryValue\n"));
    goto quit;
  }

  WINPCAP_TRACE (("sub-block '%s' -> '%.*s'\n",
                  sub_block, bytes_read, (const char*)res_buf));

  if (strlen(res_buf) >= version_buf_len)
  {
    WINPCAP_TRACE (("input buffer too small\n"));
    goto quit;
  }
  rc = TRUE;
  strcpy (version_buf, res_buf);

quit:
  if (vff_info)
     free (vff_info);
  FreeLibrary (mod);
  return (rc);
}

/**
 * Opens an adapter using the NPF device driver.
 *
 * \param AdapterName A string containing the name of the device to open.
 * \return If the function succeeds, the return value is the pointer
 *         to a properly initialized ADAPTER object, otherwise the return
 *         value is NULL.
 *
 * \note internal function used by PacketOpenAdapter() and AddAdapter().
 */
static ADAPTER *PacketOpenAdapterNPF (const char *AdapterName)
{
  ADAPTER  *lpAdapter;
  BOOL      QuerySStat, Result;
  DWORD     error, KeyRes;
  SC_HANDLE svcHandle = NULL;
  HKEY      PathKey;
  char      SymbolicLink[128], *npf;
  SERVICE_STATUS SStat;
  SC_HANDLE      scmHandle;
  SC_HANDLE      srvHandle;

  trace_func = "PacketOpenAdapterNPF";

  WINPCAP_TRACE (("\n"));

  scmHandle = OpenSCManager (NULL, NULL, GENERIC_READ);
  if (!scmHandle)
  {
    error = GetLastError();
    WINPCAP_TRACE (("OpenSCManager failed; %s\n", win_strerror(error)));
  }
  else
  {
    /* Check if the NPF registry key is already present.
     * This means that the driver is already installed and that
     * we don't need to call PacketInstallDriver
     */
    KeyRes = RegOpenKeyEx (HKEY_LOCAL_MACHINE, NPF_registry_location,
                           0, KEY_READ, &PathKey);
    if (KeyRes != ERROR_SUCCESS)
    {
      Result = PacketInstallDriver (scmHandle, &svcHandle);
    }
    else
    {
      Result = TRUE;
      RegCloseKey (PathKey);
    }

    trace_func = "PacketOpenAdapterNPF";

    if (Result)
    {
      srvHandle = OpenService (scmHandle, NPF_service_name,
                               SERVICE_START | SERVICE_QUERY_STATUS);
      if (srvHandle)
      {
        QuerySStat = QueryServiceStatus (srvHandle, &SStat);

        switch (SStat.dwCurrentState)
        {
          case SERVICE_CONTINUE_PENDING:
               WINPCAP_TRACE (("SERVICE_CONTINUE_PENDING\n"));
               break;
          case SERVICE_PAUSE_PENDING:
               WINPCAP_TRACE (("SERVICE_PAUSE_PENDING\n"));
               break;
          case SERVICE_PAUSED:
               WINPCAP_TRACE (("SERVICE_PAUSED\n"));
               break;
          case SERVICE_RUNNING:
               WINPCAP_TRACE (("SERVICE_RUNNING\n"));
               break;
          case SERVICE_START_PENDING:
               WINPCAP_TRACE (("SERVICE_START_PENDING\n"));
               break;
          case SERVICE_STOP_PENDING:
               WINPCAP_TRACE (("SERVICE_STOP_PENDING\n"));
               break;
          case SERVICE_STOPPED:
               WINPCAP_TRACE (("SERVICE_STOPPED\n"));
               break;
          default:
               WINPCAP_TRACE (("state unknown %98lX\n",
                                SStat.dwCurrentState));
               break;
        }

        if (!QuerySStat || SStat.dwCurrentState != SERVICE_RUNNING)
        {
          WINPCAP_TRACE (("Calling startservice\n"));
          if (!StartService (srvHandle, 0, NULL))
          {
            error = GetLastError();
            if (error != ERROR_SERVICE_ALREADY_RUNNING &&
                error != ERROR_ALREADY_EXISTS)
            {
              error = GetLastError();
              if (scmHandle)
                 CloseServiceHandle (scmHandle);
              scmHandle = NULL;
              WINPCAP_TRACE (("StartService failed; %s\n",
                              win_strerror(error)));
              SetLastError (error);
              return (NULL);
            }
          }
        }
        CloseServiceHandle (srvHandle);
        srvHandle = NULL;
      }
      else
      {
        error = GetLastError();
        WINPCAP_TRACE (("OpenService failed; %s\n", win_strerror(error)));
        SetLastError (error);
      }
    }
    else
    {
      if (KeyRes != ERROR_SUCCESS)
           Result = PacketInstallDriver (scmHandle, &svcHandle);
      else Result = TRUE;

      trace_func = "PacketOpenAdapterNPF";

      if (Result)
      {
        srvHandle = OpenService (scmHandle, NPF_service_name, SERVICE_START);
        if (srvHandle)
        {
          QuerySStat = QueryServiceStatus (srvHandle, &SStat);

          switch (SStat.dwCurrentState)
          {
            case SERVICE_CONTINUE_PENDING:
                 WINPCAP_TRACE (("SERVICE_CONTINUE_PENDING\n"));
                 break;
            case SERVICE_PAUSE_PENDING:
                 WINPCAP_TRACE (("SERVICE_PAUSE_PENDING\n"));
                 break;
            case SERVICE_PAUSED:
                 WINPCAP_TRACE (("SERVICE_PAUSED\n"));
                 break;
            case SERVICE_RUNNING:
                 WINPCAP_TRACE (("SERVICE_RUNNING\n"));
                 break;
            case SERVICE_START_PENDING:
                 WINPCAP_TRACE (("SERVICE_START_PENDING\n"));
                 break;
            case SERVICE_STOP_PENDING:
                 WINPCAP_TRACE (("SERVICE_STOP_PENDING\n"));
                 break;
            case SERVICE_STOPPED:
                 WINPCAP_TRACE (("SERVICE_STOPPED\n"));
                 break;
            default:
                 WINPCAP_TRACE (("state unknown 0x%08lX\n",
                                 SStat.dwCurrentState));
                 break;
          }

          if (!QuerySStat || SStat.dwCurrentState != SERVICE_RUNNING)
          {
            WINPCAP_TRACE (("Calling startservice\n"));

            if (StartService (srvHandle, 0, NULL) == 0)
            {
              error = GetLastError();
              if (error != ERROR_SERVICE_ALREADY_RUNNING &&
                  error != ERROR_ALREADY_EXISTS)
              {
                if (scmHandle)
                   CloseServiceHandle (scmHandle);
                WINPCAP_TRACE (("StartService failed; %s\n",
                                win_strerror(error)));
                SetLastError (error);
                return (NULL);
              }
            }
          }
          CloseServiceHandle (srvHandle);
        }
        else
        {
          error = GetLastError();
          WINPCAP_TRACE (("OpenService failed; %s", win_strerror(error)));
          SetLastError (error);
        }
      }
    }
  }

  if (scmHandle)
     CloseServiceHandle (scmHandle);

  lpAdapter = GlobalAllocPtr (GMEM_MOVEABLE | GMEM_ZEROINIT,
                              sizeof(*lpAdapter));
  if (!lpAdapter)
  {
    WINPCAP_TRACE (("Failed to allocate the adapter structure\n"));
    return (NULL);
  }

  /* skip "\Device" and create a symlink name
   */
  npf = strstr (AdapterName, "\\NPF_");
  _snprintf (SymbolicLink, sizeof(SymbolicLink), "\\\\.\\%s", npf);

  /* try if it is possible to open the adapter immediately
   */
  lpAdapter->hFile = CreateFile (SymbolicLink, GENERIC_WRITE | GENERIC_READ,
                                 0, NULL, OPEN_EXISTING, 0, 0);

  if (lpAdapter->hFile != INVALID_HANDLE_VALUE)
  {
    if (!PacketSetReadEvt(lpAdapter))
    {
      error = GetLastError();
      GlobalFreePtr (lpAdapter);
      WINPCAP_TRACE (("failed; %s\n", win_strerror(error)));
      SetLastError (error);
      return (NULL);
    }

    PacketSetMaxLookaheadsize (lpAdapter);
    trace_func = "PacketOpenAdapterNPF";
    StrLcpy (lpAdapter->Name, AdapterName, sizeof(lpAdapter->Name));
    return (lpAdapter);
  }

  error = GetLastError();
  GlobalFreePtr (lpAdapter);
  WINPCAP_TRACE (("CreateFile failed; %s\n", win_strerror(error)));
  SetLastError (error);
  return (NULL);
}

/**
 * Return a string with the version of the NPF.sys device driver.
 */
const char *PacketGetDriverVersion (void)
{
  return (npf_driver_version);
}

#ifdef NOT_NEEDED
/**
 * Stops and unloads the WinPcap device driver.
 *
 * \return If the function succeeds, the return value is nonzero,
 *         otherwise it is zero.
 *
 * This function can be used to unload the driver from memory when the
 * application no more needs it. Note that the driver is physically stopped
 * and unloaded only when all the files on its devices are closed, i.e. when
 * all the applications that use WinPcap close all their adapters.
 */
BOOL PacketStopDriver (void)
{
  SC_HANDLE scmHandle;
  SC_HANDLE schService;
  BOOL      rc = FALSE;

  trace_func = "PacketStopDriver";

  scmHandle = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (scmHandle)
  {
    schService = OpenService (scmHandle, NPF_service_name, SERVICE_ALL_ACCESS);
    if (schService)
    {
      SERVICE_STATUS status;

      rc = ControlService (schService, SERVICE_CONTROL_STOP, &status);
      CloseServiceHandle (schService);
      CloseServiceHandle (scmHandle);
    }
  }

  WINPCAP_TRACE (("rc %d\n", rc));
  return (rc);
}
#endif

/**
 * Opens an adapter.
 *
 * \param AdapterName A string containing the name of the device to open.
 *
 * \return If the function succeeds, the return value is the pointer to a
 * properly initialized ADAPTER object, otherwise the return value is NULL.
 */
const ADAPTER *PacketOpenAdapter (const char *AdapterName)
{
  trace_func = "PacketOpenAdapter";
  WINPCAP_TRACE (("trying to open the adapter %s\n", AdapterName));

  return PacketOpenAdapterNPF (AdapterName);
}

/**
 * Closes an adapter.
 *
 * Closes the given adapter and frees the associated
 * ADAPTER structure.
 */
void PacketCloseAdapter (ADAPTER *adapter)
{
  BOOL rc;
  int  err = 0;

  trace_func = "PacketCloseAdapter";

  if (!adapter || adapter->hFile == INVALID_HANDLE_VALUE)
  {
    WINPCAP_TRACE (("adapter already closed\n"));
    return;
  }
  WINPCAP_TRACE (("adapter file 0x%08lX\n", (DWORD)adapter->hFile));

  rc = CloseHandle (adapter->hFile);
  adapter->hFile = INVALID_HANDLE_VALUE;
  if (!rc)
     err = GetLastError();

  SetEvent (adapter->ReadEvent);   /* might already be set */
  CloseHandle (adapter->ReadEvent);
  GlobalFreePtr (adapter);

  WINPCAP_TRACE (("CloseHandle() rc %d: %s\n",
                  rc, !rc ? win_strerror(err) : "okay"));
}

/**
 * Read data packets from the NPF driver.
 *
 * The data received with this function can be a group of packets.
 * The number of packets received with this function is variable. It
 * depends on the number of packets currently stored in the driver's
 * buffer, on the size of these packets and on 'len' paramter.
 *
 * Each packet (inside 'pkt->Buffer') has a header consisting in a 'bpf_hdr'
 * structure that defines its length and timestamp. A padding field
 * is used to word-align the data in the buffer (to speed up the access
 * to the packets). The 'bh_datalen' and 'bh_hdrlen' fields of the
 * 'bpf_hdr' structures should be used to extract the packets from the
 * paramter 'buf'.
 */
int PacketReceivePacket (const ADAPTER *adapter, PACKET *pkt, BOOL sync)
{
  HANDLE handle;
  int    rc = 0;

  if ((int)adapter->ReadTimeOut != -1)
     WaitForSingleObject (adapter->ReadEvent,
                          adapter->ReadTimeOut == 0 ?
                          INFINITE : adapter->ReadTimeOut);

  handle = adapter->hFile;
  if (handle == INVALID_HANDLE_VALUE)
       SetLastError (ERROR_INVALID_HANDLE);
  else rc = ReadFile (handle, pkt->Buffer, pkt->Length, &pkt->ulBytesReceived, NULL);

  trace_func = "PacketReceivePacket";
  WINPCAP_TRACE (("rc %d; %s\n",
                  rc, rc == 0 ? win_strerror(GetLastError()) : "okay"));
  ARGSUSED (sync);
  return (rc);
}

/**
 * Sends one packet to the network.
 */
int PacketSendPacket (const ADAPTER *AdapterObject, PACKET *pkt, BOOL sync)
{
  DWORD BytesTransfered = 0UL;
  int   rc;

  rc = WriteFile (AdapterObject->hFile, pkt->Buffer, pkt->Length, &BytesTransfered, NULL);

  trace_func = "PacketSendPacket";
  WINPCAP_TRACE (("rc %d; %s\n",
                  rc, rc == 0 ? win_strerror(GetLastError()) : "okay"));
  ARGSUSED (sync);
  return (rc);
}

/**
 * Defines the minimum amount of data that will be received in a read.
 *
 * \param AdapterObject Pointer to an ADAPTER structure
 * \param nbytes the minimum amount of data in the kernel buffer that will
 *        cause the driver to release a read on this adapter.
 * \return If the function succeeds, the return value is nonzero.
 *
 * In presence of a large value for 'nbytes', the kernel waits for the arrival
 * of several packets before copying the data to the user. This guarantees a
 * low number of system calls, i.e. lower processor usage, i.e. better
 * performance, which is a good setting for applications like sniffers. Vice
 * versa, a small value means that the kernel will copy the packets as soon
 * as the application is ready to receive them. This is suggested for real
 * time applications (like, for example, a bridge) that need the better
 * responsiveness from the kernel.
 *
 * \note: this function has effect only in Windows NTx. The driver for
 *   Windows 9x doesn't offer this possibility, therefore PacketSetMinToCopy
 *   is implemented under these systems only for compatibility.
 */
BOOL PacketSetMinToCopy (const ADAPTER *AdapterObject, int nbytes)
{
  DWORD BytesReturned;
  BOOL  rc;

  rc = DeviceIoControl (AdapterObject->hFile, pBIOCSMINTOCOPY, &nbytes,
                        4, NULL, 0, &BytesReturned, NULL);

  trace_func = "PacketSetMinToCopy";
  WINPCAP_TRACE (("nbytes %d, rc %d; %s\n",
                  nbytes, rc, !rc ? win_strerror(GetLastError()) :
                  "okay"));
  return (rc);
}

/**
 * Sets the working mode of an adapter.
 *
 * \param AdapterObject Pointer to an ADAPTER structure.
 * \param mode The new working mode of the adapter.
 * \return If the function succeeds, the return value is nonzero.
 *
 * The device driver of WinPcap has 4 working modes:
 * - Capture mode (mode = PACKET_MODE_CAPT): normal capture mode. The
 *   packets transiting on the wire are copied to the application when
 *   PacketReceivePacket() is called. This is the default working mode
 *   of an adapter.
 *
 * - Statistical mode (mode = PACKET_MODE_STAT): programmable statistical
 *   mode. PacketReceivePacket() returns, at precise intervals, statics
 *   values on the network traffic. The interval between the statistic
 *   samples is by default 1 second but it can be set to any other value
 *   (with a 1 ms precision) with the PacketSetReadTimeout() function.
 *   Two 64-bit counters are provided: the number of packets and the amount
 *   of bytes that satisfy a filter previously set with PacketSetBPF(). If
 *   no filter has been set, all the packets are counted. The counters are
 *   encapsulated in a bpf_hdr structure, so that they will be parsed
 *   correctly by wpcap. Statistical mode has a very low impact on system
 *   performance compared to capture mode.
 *
 * - Dump mode (mode = PACKET_MODE_DUMP): the packets are dumped to disk by
 *   the driver, in libpcap format. This method is much faster than saving
 *   the packets after having captured them. No data is returned by
 *   PacketReceivePacket(). If the application sets a filter with
 *   PacketSetBPF(), only the packets that satisfy this filter are dumped
 *   to disk.
 *
 * - Statistical Dump mode (mode = PACKET_MODE_STAT_DUMP): the packets are
 *   dumped to disk by the driver, in libpcap format, like in dump mode.
 *   PacketReceivePacket() returns, at precise intervals, statics values on
 *   the network traffic and on the amount of data saved to file, in a way
 *   similar to statistical mode.
 *   Three 64-bit counters are provided: the number of packets accepted, the
 *   amount of bytes accepted and the effective amount of data (including
 *   headers) dumped to file. If no filter has been set, all the packets are
 *   dumped to disk. The counters are encapsulated in a bpf_hdr structure,
 *   so that they will be parsed correctly by wpcap. Look at the NetMeter
 *   example in the WinPcap developer's pack to see how to use statistics
 *   mode.
 */
BOOL PacketSetMode (const ADAPTER *AdapterObject, DWORD mode)
{
  DWORD BytesReturned;
  BOOL  rc;

  rc = DeviceIoControl (AdapterObject->hFile, pBIOCSMODE, &mode,
                        sizeof(mode), NULL, 0, &BytesReturned, NULL);

  trace_func = "PacketSetMode";
  WINPCAP_TRACE (("mode %lu, rc %d; %s\n",
                  mode, rc, !rc ? win_strerror(GetLastError()) : "okay"));
  return (rc);
}

/**
 * Returns the notification event associated with the read calls on an
 * adapter.
 *
 * \param AdapterObject Pointer to an ADAPTER structure.
 * \return The handle of the event that the driver signals when some data
 *         is available in the kernel buffer.
 *
 * The event returned by this function is signaled by the driver if:
 * - The adapter pointed by AdapterObject is in capture mode and an amount
 *   of data greater or equal than the one set with the PacketSetMinToCopy()
 *   function is received from the network.
 *
 * - The adapter pointed by AdapterObject is in capture mode, no data has
 *   been received from the network but the the timeout set with the
 *   PacketSetReadTimeout() function has elapsed.
 *
 * - The adapter pointed by AdapterObject is in statics mode and the the
 *   timeout set with the PacketSetReadTimeout() function has elapsed.
 *   This means that a new statistic sample is available.
 *
 * In every case, a call to PacketReceivePacket() will return immediately.
 * The event can be passed to standard Win32 functions (like
 * WaitForSingleObject or WaitForMultipleObjects) to wait until the
 * driver's buffer contains some data. It is particularly useful in GUI
 * applications that need to wait concurrently on several events.
 */
HANDLE PacketGetReadEvent (const ADAPTER *AdapterObject)
{
  return (AdapterObject->ReadEvent);
}

/**
 * Sets the timeout after which a read on an adapter returns.
 *
 * \param AdapterObject Pointer to an ADAPTER structure.
 * \param timeout indicates the timeout, in milliseconds, after which a call
 *        to PacketReceivePacket() on the adapter pointed by AdapterObject
 *        will be released, also if no packets have been captured by the
 *        driver. Setting timeout to 0 means no timeout, i.e.
 *        PacketReceivePacket() never returns if no packet arrives.
 *        A timeout of -1 causes PacketReceivePacket() to always return
 *        immediately.
 * \return If the function succeeds, the return value is nonzero.
 *
 * \note This function works also if the adapter is working in statistics
 *       mode, and can be used to set the time interval between two
 *       statistic reports.
 */
BOOL PacketSetReadTimeout (ADAPTER *AdapterObject, int timeout)
{
  DWORD BytesReturned;
  int   driverTimeout = -1;  /* NPF return immediately if a pkt is ready */
  BOOL  rc;

  AdapterObject->ReadTimeOut = timeout;

  rc = DeviceIoControl (AdapterObject->hFile, pBIOCSRTIMEOUT, &driverTimeout,
                        sizeof(driverTimeout), NULL, 0, &BytesReturned, NULL);

  trace_func = "PacketSetReadTimeout";
  WINPCAP_TRACE (("timeout %d, rc %d; %s\n",
                  timeout, rc, !rc ? win_strerror(GetLastError()) : "okay"));
  return (rc);
}

/**
 * Sets the size of the kernel-level buffer associated with a capture.
 *
 * \param AdapterObject Pointer to an ADAPTER structure.
 * \param dim New size of the buffer, in \b kilobytes.
 * \return The function returns TRUE if successfully completed, FALSE if
 *         there is not enough memory to allocate the new buffer.
 *
 * When a new dimension is set, the data in the old buffer is discarded and
 * the packets stored in it are lost.
 *
 * Note: the dimension of the kernel buffer affects heavily the performances
 * of the capture process. An adequate buffer in the driver is able to keep
 * the packets while the application is busy, compensating the delays of the
 * application and avoiding the loss of packets during bursts or high network
 * activity. The buffer size is set to 0 when an instance of the driver is
 * opened: the programmer should remember to set it to a proper value. As an
 * example, wpcap sets the buffer size to 1MB at the beginning of a capture.
 */
BOOL PacketSetBuff (const ADAPTER *AdapterObject, DWORD dim)
{
  DWORD BytesReturned;
  BOOL  rc;

  rc = DeviceIoControl (AdapterObject->hFile, pBIOCSETBUFFERSIZE,
                        &dim, sizeof(dim), NULL, 0, &BytesReturned, NULL);

  trace_func = "PacketSetBuff";
  WINPCAP_TRACE (("size %lu, rc %d; %s\n",
                  dim, rc, !rc ? win_strerror(GetLastError()) : "okay"));
  return (rc);
}

#ifdef NOT_NEEDED
/**
 * Sets a kernel-level packet filter.
 *
 * \param AdapterObject Pointer to an ADAPTER structure.
 * \param fp Pointer to a filtering program that will be associated
 *        with this capture or monitoring instance and that will be
 *        executed on every incoming packet.
 * \return This function returns TRUE if the filter is set successfully,
 *        FALSE if an error occurs or if the filter program is not accepted
 *        after a safeness check by the driver.  The driver performs the check
 *        in order to avoid system crashes due to buggy or malicious filters,
 *        and it rejects non conformat filters.
 *
 * This function associates a new BPF filter to the adapter AdapterObject.
 * The filter, pointed by fp, is a set of bpf_insn instructions.
 *
 * A filter can be automatically created by using the pcap_compile() function
 * of wpcap. This function converts a human readable text expression with the
 * syntax of WinDump (see the manual of WinDump at http://netgroup.polito.it/windump
 * for details) into a BPF program. If your program doesn't link wpcap, but
 * you need to know the code of a particular filter, you can launch WinDump
 * with the -d or -dd or -ddd flags to obtain the pseudocode.
 */
BOOL PacketSetBpf (const ADAPTER *AdapterObject, const struct bpf_program *fp)
{
  DWORD BytesReturned;
  BOOL  rc;

  rc = DeviceIoControl (AdapterObject->hFile, pBIOCSETF, (char*)fp->bf_insns,
                        fp->bf_len * sizeof(struct bpf_insn), NULL, 0,
                        &BytesReturned, NULL);

  trace_func = "PacketSetBpf";
  WINPCAP_TRACE (("rc %d; %s\n",
                  rc, !rc ? win_strerror(GetLastError()) : "okay"));
  return (rc);
}
#endif

/**
 * Returns statistic values about the current capture session.
 *
 * \param AdapterObject Pointer to an ADAPTER structure.
 * \param s Pointer to a user provided bpf_stat structure that will be
 *        filled by the function.
 * \return If the function succeeds, the return value is nonzero.
 */
BOOL PacketGetStatsEx (const ADAPTER *AdapterObject, struct bpf_stat *s)
{
  struct bpf_stat tmp_stat;
  DWORD  BytesReturned;
  BOOL   rc;

  memset (&tmp_stat, 0, sizeof(tmp_stat));
  rc = DeviceIoControl (AdapterObject->hFile,
                        pBIOCGSTATS, NULL, 0, &tmp_stat, sizeof(tmp_stat),
                        &BytesReturned, NULL);

  trace_func = "PacketGetStatsEx";
  WINPCAP_TRACE (("rc %d; %s\n",
                  rc, !rc ? win_strerror(GetLastError()) : "okay"));

  s->bs_recv   = tmp_stat.bs_recv;
  s->bs_drop   = tmp_stat.bs_drop;
  s->ps_ifdrop = tmp_stat.ps_ifdrop;
  s->bs_capt   = tmp_stat.bs_capt;
  return (rc);
}

/**
 * Performs a query/set operation on an internal variable of the
 * network card driver.
 *
 * \param AdapterObject Pointer to an ADAPTER structure.
 * \param set determines if the operation is a set (set = TRUE) or a
 *        query (set = FALSE).
 * \param oidData A pointer to a PACKET_OID_DATA structure that contains or
 *        receives the data.
 * \return If the function succeeds, the return value is nonzero.
 *
 * \note Not all the network adapters implement all the query/set functions.
 *
 * There is a set of mandatory OID functions that is granted to be present on
 * all the adapters, and a set of facultative functions, not provided by all
 * the cards (see the Microsoft DDKs to see which functions are mandatory).
 * If you use a facultative function, be careful to enclose it in an if
 * statement to check the result.
 */
BOOL PacketRequest (const ADAPTER *AdapterObject, BOOL set,
                    PACKET_OID_DATA *oidData)
{
  DWORD  transfered, in_size, out_size;
  DWORD  in_len, out_len;
  BOOL   rc;
  int    error = 0;
  char   prof_buf[100] = "";

#if defined(USE_PROFILER) && defined(USE_DEBUG)
  uint64 start = U64_SUFFIX (0);

  if (profile_enable && trace_file)
     start = get_rdtsc();
#endif

  transfered = 0;
  in_len   = oidData->Length;
  in_size  = sizeof(*oidData) - 1 + oidData->Length;
  out_size = in_size;
  rc = DeviceIoControl (AdapterObject->hFile,
                        set ? pBIOCSETOID : pBIOCQUERYOID,
                        oidData, in_size,
                        oidData, out_size,
                        &transfered, NULL);
  out_len = rc ? oidData->Length : 0;

  if (!rc)
     error = GetLastError();
  trace_func = "PacketRequest";

#if defined(USE_PROFILER) && defined(USE_DEBUG)
  if (profile_enable && trace_file)
  {
    int64  clocks = (int64) (get_rdtsc() - start);
    double msec   = (double)clocks / ((double)clocks_per_usec * 1000.0);
    _snprintf (prof_buf, sizeof(prof_buf), " (%.3f ms)", msec);
  }
#endif

  WINPCAP_TRACE (("OID 0x%08lX, len %lu/%lu, xfered %lu,%s set %d, rc %d; %s\n",
                  oidData->Oid, in_len, out_len, transfered, prof_buf, set,
                  rc, !rc ? win_strerror(error) : "okay"));
  if (!rc)
     SetLastError (error);
  return (rc);
}

/*
 * Formerly in AdInfo.c:
 *
 * This file contains the support functions used by packet.dll to retrieve
 * information about installed adapters, like
 *
 * - the adapter list
 * - the device associated to any adapter and the description of the adapter
 */

/**
 * Scan the registry to retrieve the IP addresses of an adapter.
 *
 * \param AdapterName String that contains the name of the adapter.
 * \param buffer A user allocated array of npf_if_addr that will be filled
 *        by the function.
 * \param NEntries Size of the array (in npf_if_addr).
 * \return If the function succeeds, the return value is nonzero.
 *
 * This function grabs from the registry information like the IP addresses,
 * the netmasks and the broadcast addresses of an interface. The buffer
 * passed by the user is filled with npf_if_addr structures, each of which
 * contains the data for a single address. If the buffer is full, the reaming
 * addresses are dropeed, therefore set its dimension to sizeof(npf_if_addr)
 * if you want only the first address.
 */
static BOOL GetAddressesFromRegistry (const char  *AdapterName,
                                      npf_if_addr *buffer,
                                      int         *NEntries)
{
  struct sockaddr_in *TmpAddr, *TmpBroad;
  const char *guid;
  HKEY  TcpIpKey = NULL, UnderTcpKey = NULL;
  char  String [1024+1];
  DWORD RegType, BufLen, StringPos, DHCPEnabled, ZeroBroadcast;
  LONG  status;
  DWORD line = 0;
  int   naddrs, nmasks;

  *NEntries = 0;

  trace_func = "GetAddressesFromRegistry";

  guid = strchr (AdapterName, '{');

  WINPCAP_TRACE (("guid %s\n", guid));

  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
       "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces",
       0, KEY_READ, &UnderTcpKey) == ERROR_SUCCESS)
  {
    status = RegOpenKeyEx (UnderTcpKey, guid, 0, KEY_READ, &TcpIpKey);
    line = __LINE__ - 1;
    if (status != ERROR_SUCCESS)
       goto fail;
  }
  else
  {
    HKEY SystemKey, ParametersKey, InterfaceKey;

    /* Query the registry key with the interface's addresses
     */
    status = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                           "SYSTEM\\CurrentControlSet\\Services",
                           0, KEY_READ, &SystemKey);
    if (status != ERROR_SUCCESS)
       goto fail;

    status = RegOpenKeyEx (SystemKey, guid, 0, KEY_READ, &InterfaceKey);
    line = __LINE__ - 1;
    RegCloseKey (SystemKey);
    if (status != ERROR_SUCCESS)
       goto fail;

    status = RegOpenKeyEx (InterfaceKey, "Parameters", 0, KEY_READ,
                           &ParametersKey);
    line = __LINE__ - 1;
    RegCloseKey (InterfaceKey);
    if (status != ERROR_SUCCESS)
       goto fail;

    status = RegOpenKeyEx (ParametersKey, "TcpIp", 0, KEY_READ, &TcpIpKey);
    line = __LINE__ - 1;
    RegCloseKey (ParametersKey);
    if (status != ERROR_SUCCESS)
       goto fail;

    BufLen = sizeof(String);
  }

  BufLen = 4;

  /* Try to detect if the interface has a zero broadcast addr
   */
  status = RegQueryValueEx (TcpIpKey, "UseZeroBroadcast", NULL,
                            &RegType, (BYTE*)&ZeroBroadcast, &BufLen);
  if (status != ERROR_SUCCESS)
     ZeroBroadcast = 0;

  BufLen = 4;

  /* See if DHCP is used by this system
   */
  status = RegQueryValueEx (TcpIpKey, "EnableDHCP", NULL,
                            &RegType, (BYTE*)&DHCPEnabled, &BufLen);
  if (status != ERROR_SUCCESS)
     DHCPEnabled = 0;

  /* Retrieve the addresses
   */
  if (DHCPEnabled)
  {
    WINPCAP_TRACE (("  DHCP enabled\n"));

    BufLen = sizeof(String);
    status = RegQueryValueEx (TcpIpKey, "DhcpIPAddress", NULL,
                              &RegType, (BYTE*)String, &BufLen);
    line = __LINE__ - 1;
    if (status != ERROR_SUCCESS)
       goto fail;

    /* scan the key to obtain the addresses
     */
    StringPos = 0;
    for (naddrs = 0; naddrs < MAX_NETWORK_ADDRESSES; naddrs++)
    {
      TmpAddr = (struct sockaddr_in*) &buffer[naddrs].IPAddress;
      WINPCAP_TRACE (("  addr %s\n", String+StringPos));

      if (inet_aton(String + StringPos, &TmpAddr->sin_addr))
      {
        TmpAddr->sin_family = AF_INET;
        TmpBroad = (struct sockaddr_in*) &buffer[naddrs].Broadcast;
        TmpBroad->sin_family = AF_INET;
        if (!ZeroBroadcast)
             TmpBroad->sin_addr.s_addr = INADDR_BROADCAST;
        else TmpBroad->sin_addr.s_addr = INADDR_ANY;

        while (*(String+StringPos))
             StringPos++;
        StringPos++;

        if (*(String + StringPos) == '\0' || StringPos >= BufLen)
           break;
      }
      else
        break;
    }

    BufLen = sizeof(String);

    /* Open the key with the netmasks
     */
    status = RegQueryValueEx (TcpIpKey, "DhcpSubnetMask", NULL,
                              &RegType, (BYTE*)String, &BufLen);
    line = __LINE__ - 1;
    if (status != ERROR_SUCCESS)
       goto fail;

    /* scan the key to obtain the masks
     */
    StringPos = 0;
    for (nmasks = 0; nmasks < MAX_NETWORK_ADDRESSES; nmasks++)
    {
      TmpAddr = (struct sockaddr_in*) &buffer[nmasks].SubnetMask;
      WINPCAP_TRACE (("  addr %s\n", String+StringPos));

      if (inet_aton (String + StringPos, &TmpAddr->sin_addr))
      {
        TmpAddr->sin_family = AF_INET;

        while (*(String + StringPos))
              StringPos++;
        StringPos++;
        if (*(String + StringPos) == '\0' || StringPos >= BufLen)
           break;
      }
      else
        break;
    }

    /* The number of masks MUST be equal to the number of addresses
     */
    if (nmasks != naddrs)
       goto fail;
  }
  else   /* Not DHCP enabled */
  {
    WINPCAP_TRACE (("  Not DHCP enabled\n"));

    BufLen = sizeof(String);

    /* Open the key with the addresses
     */
    status = RegQueryValueEx (TcpIpKey, "IPAddress", NULL,
                              &RegType, (BYTE*)String, &BufLen);
    line = __LINE__ - 1;
    if (status != ERROR_SUCCESS)
       goto fail;

    /* scan the key to obtain the addresses
     */
    StringPos = 0;
    for (naddrs = 0; naddrs < MAX_NETWORK_ADDRESSES; naddrs++)
    {
      TmpAddr = (struct sockaddr_in*) &buffer[naddrs].IPAddress;
      WINPCAP_TRACE (("  addr %s\n", String+StringPos));

      if (inet_aton (String + StringPos, &TmpAddr->sin_addr))
      {
        TmpAddr->sin_family = AF_INET;

        TmpBroad = (struct sockaddr_in*) &buffer[naddrs].Broadcast;
        TmpBroad->sin_family = AF_INET;
        if (!ZeroBroadcast)
             TmpBroad->sin_addr.s_addr = INADDR_BROADCAST;
        else TmpBroad->sin_addr.s_addr = INADDR_ANY;

        while (*(String + StringPos))
              StringPos++;
        StringPos++;

        if (*(String + StringPos) == '\0' || StringPos >= BufLen)
           break;
      }
      else
        break;
    }

    /* Open the key with the netmasks
     */
    BufLen = sizeof(String);
    status = RegQueryValueEx (TcpIpKey, "SubnetMask", NULL,
                              &RegType, (BYTE*)String, &BufLen);
    line = __LINE__ - 1;
    if (status != ERROR_SUCCESS)
       goto fail;

    /* scan the key to obtain the masks
     */
    StringPos = 0;
    for (nmasks = 0; nmasks < MAX_NETWORK_ADDRESSES; nmasks++)
    {
      TmpAddr = (struct sockaddr_in*) &buffer[nmasks].SubnetMask;
      WINPCAP_TRACE (("  mask %s\n", String+StringPos));

      if (inet_aton (String + StringPos, &TmpAddr->sin_addr))
      {
        TmpAddr->sin_family = AF_INET;

        while (*(String + StringPos) != 0)
              StringPos++;
        StringPos++;

        if (*(String + StringPos) == 0 || StringPos >= BufLen)
           break;
      }
      else
        break;
    }

    /* The number of masks MUST be equal to the number of addresses
     */
    if (nmasks != naddrs)
       goto fail;
  }

  *NEntries = naddrs + 1;

  RegCloseKey (TcpIpKey);
  RegCloseKey (UnderTcpKey);

  return (status != ERROR_SUCCESS);

fail:
  WINPCAP_TRACE (("failed line %u; %s\n", line, win_strerror(GetLastError())));

  if (TcpIpKey)
     RegCloseKey (TcpIpKey);
  if (UnderTcpKey)
     RegCloseKey (UnderTcpKey);
  *NEntries = 0;
  ARGSUSED (line);
  return (FALSE);
}

/**
 * Adds an entry to the 'adapters_list' list.
 *
 * \param AdName Name of the adapter to add
 * \return If the function succeeds, the return value is nonzero.
 *
 * Used by PacketGetAdapters(). Queries the registry to fill the
 * '*ADAPTER_INFO' describing the new adapter.
 */
static BOOL AddAdapter (const char *AdName)
{
  struct {
    PACKET_OID_DATA oidData;
    char            descr[512];
  } oid;

  ADAPTER_INFO *TmpAdInfo = NULL;
  ADAPTER      *adapter   = NULL;
  char         *descr, *p;
  LONG          Status;
  BOOL          rc = FALSE;

  trace_func = "AddAdapter";

  WINPCAP_TRACE (("adapter %s\n", AdName));

  WaitForSingleObject (adapters_mutex, INFINITE);

  for (TmpAdInfo = adapters_list; TmpAdInfo; TmpAdInfo = TmpAdInfo->Next)
  {
    if (!strcmp(AdName, TmpAdInfo->Name))
    {
      WINPCAP_TRACE (("adapter already present in the list\n"));
      ReleaseMutex (adapters_mutex);
      return (TRUE);
    }
  }

  TmpAdInfo = NULL;

  /* Here we could have released the mutex, but what happens if two
   * threads try to add the same adapter? The adapter would be duplicated
   * on the linked list.
   */

  /* Try to Open the adapter
   */
  adapter = PacketOpenAdapterNPF (AdName);
  trace_func = "AddAdapter";

  if (!adapter)
     goto quit;

  /* PacketOpenAdapter() was succesful. Consider this a valid adapter
   * and allocate an entry for it In the adapter list.
   */
  TmpAdInfo = GlobalAllocPtr (GMEM_MOVEABLE | GMEM_ZEROINIT,
                              sizeof(*TmpAdInfo));
  if (!TmpAdInfo)
  {
    WINPCAP_TRACE (("GlobalAllocPtr Failed\n"));
    goto quit;
  }

  /**< \todo; keep 'AdName' in 'TmpAdInfo->Name' only */
#if 0
  adapter->info = TmpAdInfo;
#endif

  /* Copy the device name
   */
  strcpy (TmpAdInfo->Name, AdName);

  /* Query the NIC driver for the adapter description
   */
  memset (&oid, 0, sizeof(oid));
  oid.oidData.Oid    = OID_GEN_VENDOR_DESCRIPTION;
  oid.oidData.Length = sizeof(oid.descr);

  Status = PacketRequest (adapter, FALSE, &oid.oidData);
  trace_func = "AddAdapter";

  descr = (char*) oid.oidData.Data;

  if (!Status || !*descr)
     WINPCAP_TRACE (("failed to get adapter description\n"));

  /* Strip trailing newlines and right aligned text in description;
   * text separated by TABs or >= 2 spaces.
   */
  p = strrchr (descr, '\r');
  if (p)
     *p = '\0';
  p = strchr (descr, '\t');
  if (p)
     *p = '\0';
  p = strstr (descr, "  ");
  if (p)
     *p = '\0';

  WINPCAP_TRACE (("adapter Description \"%s\"\n", descr));

  /* Save the description
   */
  strncpy (TmpAdInfo->Description, descr, sizeof(TmpAdInfo->Description));

  GetAddressesFromRegistry (TmpAdInfo->Name,
                            TmpAdInfo->NetworkAddresses,
                            &TmpAdInfo->NNetworkAddresses);

  /* Update the AdaptersInfo list
   */
  TmpAdInfo->Next  = adapters_list;
  adapters_list = TmpAdInfo;

  rc = TRUE;

quit:
  /* Close it; we don't need it until user open it again.
   */
  if (adapter)
     PacketCloseAdapter (adapter);
  if (!rc && TmpAdInfo)
     GlobalFreePtr (TmpAdInfo);
  ReleaseMutex (adapters_mutex);
  return (rc);
}

/**
 * Updates the list of the adapters querying the registry.
 *
 * \return If the function succeeds, the return value is nonzero.
 *
 * This function populates the list of adapter descriptions, retrieving
 * the information from the registry.
 */
static BOOL PacketGetAdapters (void)
{
  HKEY  LinkageKey, AdapKey, OneAdapKey;
  DWORD RegKeySize = 0;
  DWORD dim, RegType;
  char  TName[256];
  char  TAName[256];
  char  AdapName[256];
  const char *guid;
  LONG  Status;
  int   i = 0;

  trace_func = "PacketGetAdapters";
  WINPCAP_TRACE (("\n"));

  Status = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                         "SYSTEM\\CurrentControlSet\\Control\\Class\\"
                         "{4D36E972-E325-11CE-BFC1-08002BE10318}",
                         0, KEY_READ, &AdapKey);
  i = 0;

  if (Status != ERROR_SUCCESS)
  {
    WINPCAP_TRACE (("RegOpenKeyEx ( Class\\{networkclassguid} ) Failed\n"));
    goto nt4;
  }

  WINPCAP_TRACE (("cycling through the adapters:\n"));

  /* Cycle through the entries inside the {4D3..} key
   * to get the names of the adapters
   */
  while (RegEnumKey (AdapKey, i, AdapName, sizeof(AdapName)) == ERROR_SUCCESS)
  {
    WINPCAP_TRACE (("  loop %d: sub-key %s\n", i, AdapName));
    i++;

    /* Get the adapter name from the registry key
     */
    Status = RegOpenKeyEx (AdapKey, AdapName, 0, KEY_READ, &OneAdapKey);
    if (Status != ERROR_SUCCESS)
    {
      WINPCAP_TRACE (("  RegOpenKeyEx (OneAdapKey) Failed\n"));
      continue;
    }

    Status = RegOpenKeyEx (OneAdapKey, "Linkage", 0, KEY_READ, &LinkageKey);
    if (Status != ERROR_SUCCESS)
    {
      RegCloseKey (OneAdapKey);
      WINPCAP_TRACE (("  RegOpenKeyEx (LinkageKey) Failed\n"));
      continue;
    }

    dim = sizeof(TName);
    Status = RegQueryValueEx (LinkageKey, "Export", NULL, NULL,
                              (BYTE*)TName, &dim);
    RegCloseKey (OneAdapKey);
    RegCloseKey (LinkageKey);
    if (Status != ERROR_SUCCESS)
    {
      WINPCAP_TRACE (("  Name = SKIPPED (error reading the key)\n"));
      continue;
    }
    if (!TName[0])
    {
      WINPCAP_TRACE (("  Name = SKIPPED (empty name)\n"));
      continue;
    }
    guid = strchr (TName, '{');
    if (!guid)
    {
      WINPCAP_TRACE (("  Name = SKIPPED (missing GUID)\n"));
      continue;
    }

    WINPCAP_TRACE (("  key %d: %s\n", i, TName));

    /* Put the \Device\NPF_ string at the beginning of the name
     */
    _snprintf (TAName, sizeof(TAName), "\\Device\\NPF_%s", guid);

    /* If the adapter is valid, add it to the list.
     */
    AddAdapter (TAName);
    trace_func = "PacketGetAdapters";
  }

  RegCloseKey (AdapKey);

nt4:

  /*
   * No adapters were found under {4D36E972-E325-11CE-BFC1-08002BE10318}.
   * This means with great probability that we are under Win-NT 4, so we
   * try to look under the tcpip bindings.
   */

  if (i == 0)
       WINPCAP_TRACE (("Adapters not found under SYSTEM\\CurrentControlSet\\"
                       "Control\\Class. Using the TCP/IP bindings.\n"));
  else WINPCAP_TRACE (("Trying NT4 bindings anyway\n"));

  Status = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                         "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Linkage",
                         0, KEY_READ, &LinkageKey);
  if (Status == ERROR_SUCCESS)
  {
    BYTE *bpStr;

    /* Retrieve the length of the binding key
     */
    Status = RegQueryValueEx (LinkageKey, "bind", NULL, &RegType,
                              NULL, &RegKeySize);

    bpStr = calloc (RegKeySize+1, 1);
    if (!bpStr)
       return (FALSE);

    /* Query the key again to get its content
     */
    Status = RegQueryValueEx (LinkageKey, "bind", NULL, &RegType,
                              bpStr, &RegKeySize);
    RegCloseKey (LinkageKey);

    /* Loop over the space separated "\Device\{.." strings.
     */
    for (guid = strchr(bpStr,'{'); guid; guid = strchr(guid+1,'{'))
    {
      _snprintf (TAName, sizeof(TAName), "\\Device\\NPF_%s", guid);
      AddAdapter (TAName);
      trace_func = "PacketGetAdapters";
    }
    free (bpStr);
  }
  else
  {
    WINPCAP_TRACE (("Cannot find the TCP/IP bindings"));
    return (FALSE);
  }
  return (TRUE);
}

/**
 * Find the information about an adapter scanning the global ADAPTER_INFO list.
 *
 * \param AdapterName Name of the adapter.
 * \return NULL if function fails, otherwise adapter info.
 */
const ADAPTER_INFO *PacketFindAdInfo (const char *AdapterName)
{
  const ADAPTER_INFO *ad_info;

  trace_func = "PacketFindAdInfo";
  WINPCAP_TRACE (("\n"));

  for (ad_info = adapters_list; ad_info; ad_info = ad_info->Next)
      if (!strcmp(ad_info->Name, AdapterName))
         break;
  return (ad_info);
}

/**
 * Return list of adapters.
 */
const ADAPTER_INFO *PacketGetAdInfo (void)
{
  trace_func = "PacketGetAdInfo";
  WINPCAP_TRACE (("\n"));

  return (adapters_list);
}

/**
 * Populates the list of the adapters.
 *
 * This function populates the list of adapter descriptions, invoking
 * PacketGetAdapters(). Called from PacketInitModule() only.
 */
static BOOL PopulateAdaptersInfoList (void)
{
  BOOL rc = TRUE;

  trace_func = "PopulateAdaptersInfoList";
  WINPCAP_TRACE (("\n"));

  WaitForSingleObject (adapters_mutex, INFINITE);

  adapters_list = NULL;

  /* Fill the list
   */
  if (!PacketGetAdapters())
  {
    WINPCAP_TRACE (("registry scan for adapters failed!\n"));
    rc = FALSE;
  }
  ReleaseMutex (adapters_mutex);
  return (rc);
}

/**
 * Free the list of the adapters.
 */
static BOOL FreeAdaptersInfoList (void)
{
  ADAPTER_INFO *next, *ad_info;

  trace_func = "FreeAdaptersInfoList";
  WINPCAP_TRACE (("\n"));

  WaitForSingleObject (adapters_mutex, INFINITE);

  for (ad_info = adapters_list; ad_info; ad_info = next)
  {
    next = ad_info->Next;
    GlobalFreePtr (ad_info);
  }

  adapters_list = NULL;
  ReleaseMutex (adapters_mutex);
  return (TRUE);
}
#endif   /* USE_DYN_PACKET */
#endif   /* WIN32 || _WIN32 */
