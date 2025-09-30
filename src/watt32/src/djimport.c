/*
 * import.c - import library for Watt-32/DJGPP
 *
 * written by 2002 dbjh, heavily modified by G. Vanem 2003
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__DJGPP__) && defined(WATT32_DOS_DLL)

#define MAKE_IMPORT
#include "wattcp.h"      /* dxe_sym.h is included */
#include "dxe_map.h"

#define INITIAL_HANDLE 1
#define DXE_DEBUG      1

#if (DXE_DEBUG)
  #define DXE_MSG(x)   printf x
#else
  #define DXE_MSG(x)   ((void)0)
#endif

static char dxe_module[FILENAME_MAX] = "watt32.dxe";

static void     *libwatt;
static st_map_t *dxe_map;

#ifdef HAVE_STUBS
  #include "djgpp/dxe/stubs.inc"
#else
  static int         (*__imp_sock_init) (void);
  static int         (*__imp_sock_exit) (void);
  static void        (*__imp_dbug_init) (void);
  static const char *(*__imp_wattcpVersion) (void);
  static const char *(*__imp_wattcpCapabilities) (void);
  static int         (*__imp_accept) (int, struct sockaddr *, int *);
#endif

static void *open_module (const char *module_name);
static void *get_symbol (void *handle, char *symbol_name);
static void  load_symbols (void *handle);

static void load_dxe (void)
{
  static int   loaded = 0;
  const  char *path;

  if (loaded)
     return;

  path = dxe_module;
  if (access(path,0) != 0)
     path = searchpath (path);
  if (!path)
  {
    fprintf (stderr, "Failed to find \"%s\" on $PATH\n", dxe_module);
    exit (-1);
  }

  DXE_MSG (("load_dxe(): opening %s\n", path));

  libwatt = open_module (path);
  if (!libwatt)
     exit (-1);

  load_symbols (libwatt);
  loaded = 1;
}

#ifdef HAVE_STUBS
  #include "djgpp/dxe/symbols.inc"
#else
/*
 * Generated function
 */
static void load_symbols (void *handle)
{
  __imp_accept             = get_symbol (handle, "accept");
  __imp_sock_init          = get_symbol (handle, "sock_init");
  __imp_sock_exit          = get_symbol (handle, "sock_exit");
  __imp_dbug_init          = get_symbol (handle, "dbug_init");
  __imp_wattcpVersion      = get_symbol (handle, "wattcpVersion");
  __imp_wattcpCapabilities = get_symbol (handle, "wattcpCapabilities");
}
#endif

#ifndef HAVE_STUBS

int accept (int s, struct sockaddr *addr, int *addrlen)
{
  load_dxe();
  return (*__imp_accept) (s, addr, addrlen);
}

char *addr2ascii (int af, const void *addrp, int len, char *buf)
{
}

int ascii2addr (int af, const char *ascii, void *result)
{
}

int bind (int s, const struct sockaddr *myaddr, int namelen)
{
}

int _getpeername (const _tcp_Socket *s, void *dest, int *len)
{
}

int _getsockname (const _tcp_Socket *s, void *dest, int *len)
{
}

int getdomainname (char *buffer, int buflen)
{
}

int setdomainname (const char *name, int len)
{
}

int gethostname (char *buffer, int buflen)
{
}

int sethostname (const char *fqdn, int len)
{
}

WORD in_check_sum (const void *ptr, int len)
{
}

int close_s (int s)
{
}

int closesocket (int s)
{
  return close_s (s);
}

int connect (int s, const struct sockaddr *servaddr, int addrlen)
{
}

int fcntlsocket (int s, int cmd, ...)
{
}

char *gai_strerror (int ecode)
{
}

void freeaddrinfo (struct addrinfo *ai)
{
}

int getaddrinfo (const char *hostname, const char *servname,
                 const struct addrinfo *hints, struct addrinfo **res)
{
}

int getnameinfo (const struct sockaddr *sa, int salen, char *host,
                 int hostlen, char *serv, int servlen, int flags)
{
}

struct hostent * gethostent (void)
{
}

struct hostent *gethostbyname (const char *name)
{
}

struct hostent *gethostbyname2 (const char *name, int family)
{
}

struct hostent *gethostbyaddr (const char *addr_name, int len, int type)
{
}

void sethostent (int stayopen)
{
}

void endhostent (void)
{
}

u_long gethostid (void)
{
}

int if_nametoindex (const char *cp)
{
}

char *if_indextoname (int id, const char *cp)
{
}

int getsockname (int s, struct sockaddr *name, int *namelen)
{
}

int getpeername (int s, struct sockaddr *name, int *namelen)
{
}

struct netent * getnetent (void)
{
}

struct netent * getnetbyname (const char *name)
{
}

struct netent *getnetbyaddr (long net, int type)
{
}

void setnetent (int stayopen)
{
}

void endnetent (void)
{
}

struct protoent * getprotoent (void)
{
}

struct protoent * getprotobyname (const char *proto)
{
}

struct protoent * getprotobynumber (int proto)
{
}

void setprotoent (int stayopen)
{
}

void endprotoent (void)
{
}

WORD _getshort (const BYTE *x)
{
}

DWORD _getlong (const BYTE *x)   /* in <arpa/nameserv.h> */
{
}

void __putshort (WORD var, BYTE *ptr)   /* in <resolv.h> */
{
}

void __putlong (DWORD var, BYTE *ptr)   /* in <resolv.h> */
{
}

struct servent * getservent (void)
{
}

struct servent * getservbyname (const char *serv, const char *proto)
{
}

struct servent * getservbyport (int port, const char *proto)
{
}

void setservent (int stayopen)
{
}

void endservent (void)
{
}

int gettimeofday2 (struct timeval *tv, struct timezone *tz)
{
}

/* t.o.m.  G*.C */


int sock_init (void)
{
  load_dxe();
  return (*__imp_sock_init)();
}

int sock_exit (void)
{
  load_dxe();
  return (*__imp_sock_exit)();
}

void dbug_init (void)
{
  load_dxe();
  (*__imp_dbug_init)();
}

const char * wattcpVersion (void)
{
  load_dxe();
  return (*__imp_wattcpVersion)();
}

const char * wattcpCapabilities (void)
{
  load_dxe();
  return (*__imp_wattcpCapabilities)();
}
#endif  /* HAVE_STUBS */


static void uninit_func (void)
{
  fprintf (stderr, "An uninitialized member of the import/export structure "
           "was called!\nUpdate open_module().\n");
  exit (1);
}

static void *open_module (const char *module_name)
{
  static int new_handle = INITIAL_HANDLE;
  st_symbol_t *sym = _dxe_load ((char*)module_name);
  void        *handle;
  int          n, m;

  /* _dxe_load() doesn't really return a handle. It returns a pointer
   * to the one symbol a DXE module can export.
   */
  if (!sym)
  {
    fprintf (stderr, "Error loading DXE %s: %s\n",
             module_name, strerror(errno));
    return (NULL);
  }

  /* initialize the import/export structure
   *
   * Catch calls to uninitialized members in case a new function was added to
   * st_symbol_t, but forgotten to initialize here.
   */
  m = sizeof(st_symbol_t) / sizeof(void(*)(void));

  for (n = 0; n < m && ((void (**)(void))sym)[n]; n++)
      ;
  for ( ; n < m; n++)
      ((void(**)(void)) sym)[n] = uninit_func;

  /* initialize data+functions
   */
  sym->__FSEXT_alloc_fd                            = __FSEXT_alloc_fd;
  sym->__FSEXT_get_data                            = __FSEXT_get_data;
  sym->__FSEXT_set_data                            = __FSEXT_set_data;
  sym->__FSEXT_set_function                        = __FSEXT_set_function;
  sym->__crt0_argv                                 = __crt0_argv;
  sym->__dj_assert                                 = __dj_assert;
#if 0
  sym->__dj_ctype_flags                            = __dj_ctype_flags;
  sym->__dj_ctype_tolower                          = __dj_ctype_tolower;
  sym->__dj_ctype_toupper                          = __dj_ctype_toupper;
#endif
  sym->__dj_movedata                               = __dj_movedata;
  sym->__dj_stderr                                 = __dj_stderr;
  sym->__dj_stdin                                  = __dj_stdin;
  sym->__dj_stdout                                 = __dj_stdout;
  sym->__djgpp_exception_state_ptr                 = __djgpp_exception_state_ptr;
  sym->__djgpp_set_ctrl_c                          = __djgpp_set_ctrl_c;
  sym->__dpmi_error                                = __dpmi_error;
  sym->__dpmi_get_real_mode_interrupt_vector       = __dpmi_get_real_mode_interrupt_vector;
  sym->__dpmi_get_segment_base_address             = __dpmi_get_segment_base_address;
  sym->__dpmi_get_segment_limit                    = __dpmi_get_segment_limit;
  sym->__dpmi_int                                  = __dpmi_int;
  sym->__dpmi_unlock_linear_region                 = __dpmi_unlock_linear_region;
  sym->__dpmi_yield                                = __dpmi_yield;
  sym->__movedata                                  = __movedata;
#if 0
  sym->__udivdi3                                   = __udivdi3;
  sym->__umoddi3                                   = __umoddi3;
#endif
  sym->_close                                      = _close;
  sym->_exit                                       = _exit;
  sym->_fmode                                      = _fmode;
  sym->_get_dos_version                            = _get_dos_version;
  sym->_go32_dpmi_allocate_dos_memory              = _go32_dpmi_allocate_dos_memory;
  sym->_go32_dpmi_allocate_real_mode_callback_retf = _go32_dpmi_allocate_real_mode_callback_retf;
  sym->_go32_dpmi_free_dos_memory                  = _go32_dpmi_free_dos_memory;
  sym->_go32_dpmi_free_real_mode_callback          = _go32_dpmi_free_real_mode_callback;
  sym->_go32_dpmi_lock_code                        = _go32_dpmi_lock_code;
  sym->_go32_dpmi_lock_data                        = _go32_dpmi_lock_data;
  sym->_go32_info_block                            = _go32_info_block;
  sym->_go32_want_ctrl_break                       = _go32_want_ctrl_break;
  sym->_write                                      = _write;
  sym->_chmod                                      = _chmod;
  sym->atexit                                      = atexit;
  sym->atoi                                        = atoi;
  sym->atol                                        = atol;
  sym->bsearch                                     = bsearch;
  sym->calloc                                      = calloc;
  sym->creat                                       = creat;
  sym->ctime                                       = ctime;
  sym->dosmemget                                   = dosmemget;
  sym->errno                                       = errno;
  sym->exit                                        = exit;
  sym->fclose                                      = fclose;
  sym->feof                                        = feof;
  sym->ferror                                      = ferror;
  sym->fflush                                      = fflush;
  sym->fgets                                       = fgets;
  sym->fileno                                      = fileno;
  sym->fopen                                       = fopen;
  sym->fprintf                                     = fprintf;
  sym->fputc                                       = fputc;
  sym->fputs                                       = fputs;
  sym->fread                                       = fread;
  sym->free                                        = free;
  sym->fwrite                                      = fwrite;
  sym->getc                                        = getc;
  sym->getcbrk                                     = getcbrk;
  sym->getenv                                      = getenv;
  sym->getpid                                      = getpid;
  sym->getrlimit                                   = getrlimit;
  sym->gettimeofday                                = gettimeofday;
  sym->isatty                                      = isatty;
  sym->itoa                                        = itoa;
  sym->kbhit                                       = kbhit;
  sym->localtime                                   = localtime;
  sym->longjmp                                     = longjmp;
  sym->malloc                                      = malloc;
  sym->memchr                                      = memchr;
  sym->memcpy                                      = memcpy;
  sym->_memmove                                    = memmove;
  sym->memset                                      = memset;
  sym->mktime                                      = mktime;
  sym->printf                                      = printf;
  sym->psignal                                     = psignal;
  sym->putc                                        = putc;
  sym->puts                                        = puts;
  sym->qsort                                       = qsort;
  sym->raise                                       = raise;
  sym->rand                                        = rand;
  sym->realloc                                     = realloc;
  sym->rewind                                      = rewind;
  sym->setbuf                                      = setbuf;
  sym->setitimer                                   = setitimer;
  sym->setjmp                                      = setjmp;
  sym->setrlimit                                   = setrlimit;
  sym->signal                                      = signal;
  sym->sleep                                       = sleep;
  sym->sprintf                                     = sprintf;
  sym->srand                                       = srand;
  sym->sscanf                                      = sscanf;
  sym->strcat                                      = strcat;
  sym->strchr                                      = strchr;
  sym->strcmp                                      = strcmp;
  sym->strcpy                                      = strcpy;
  sym->strdup                                      = strdup;
  sym->stricmp                                     = stricmp;
  sym->strlen                                      = strlen;
  sym->strlwr                                      = strlwr;
  sym->strncat                                     = strncat;
  sym->strncmp                                     = strncmp;
  sym->strncpy                                     = strncpy;
  sym->strnicmp                                    = strnicmp;
  sym->strrchr                                     = strrchr;
  sym->strstr                                      = strstr;
  sym->strtok                                      = strtok;
  sym->strupr                                      = strupr;
  sym->time                                        = time;
  sym->unlink                                      = unlink;
  sym->vfprintf                                    = vfprintf;
  sym->vsprintf                                    = vsprintf;
  sym->vsscanf                                     = vsscanf;

#if (__DJGPP_MINOR__ >= 4)
  sym->snprintf  = snprintf;
  sym->vsnprintf = vsnprintf;
#endif

  /* initialize the DXE module
   */
  sym->dxe_init();

  if (new_handle == INITIAL_HANDLE)
     dxe_map = map_create (50);

  dxe_map = map_put (dxe_map, (void*)new_handle, sym);

#if (DXE_DUMP)
  map_dump (dxe_map);
#endif

  handle  = (void*) new_handle++;
  return (handle);
}


static void *get_symbol (void *handle, char *symbol)
{
  void        *symptr;
  st_symbol_t *sym;

  DXE_MSG (("get_symbol(): handle %08X, sym `%s'\n", (unsigned)handle, symbol));

  sym = map_get (dxe_map, handle);
  if (!sym)
  {
    fprintf (stderr, "Invalid handle: %08X\n", (unsigned)handle);
    exit (1);
  }

  symptr = sym->dxe_symbol (symbol);
  if (!symptr)
  {
    fprintf (stderr, "Could not find symbol \"%s\"\n", symbol);
    exit (1);
  }
  return (symptr);
}

#endif  /* __DJGPP__ && WATT32_DOS_DLL */

