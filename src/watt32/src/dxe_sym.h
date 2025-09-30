/*
 * dxe_sym.h - DXE client support code
 *
 * written by 2002 dbjh
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

#ifndef _w32_DXE_SYM_H
#define _w32_DXE_SYM_H

#if !defined(WATT32_DOS_DLL)
#error You must define "WATT32_DOS_DLL" to include this file.
#endif

#define YY_ALWAYS_INTERACTIVE  1
#define ICMP_MAXTYPE           18

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <sys/dxe.h>
#include <sys/fsext.h>
#include <sys/resource.h>
#include <sys/exceptn.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <io.h>
#include <dir.h>
#include <dos.h>
#include <crt0.h>
#include <go32.h>
#include <dpmi.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <rpc/rpc.h>
#include <sys/wtime.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/route.h>
#include <net/ppp_defs.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_pcb.h>
#include <netinet/tcp.h>
#include <netinet/tcp_time.h>
#include <netinet/tcpip.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/tcp_var.h>
#include <netinet/icmp_var.h>
#include <netinet/igmp_var.h>
#include <netinet/icmp6.h>
#include <netinet6/ip6_var.h>


struct st_symbol
{
  /* Functions exported by the DXE module.
   */
  int   (*dxe_init) (void);
  void *(*dxe_symbol) (const char *symbol_name);

  /*
   * Functions imported by the DXE module.
   * Note that _every_ function used by the DXE module and not defined in it
   * should be listed here. That includes standard C library functions and
   * variables.
   */
  int            (*__FSEXT_alloc_fd) (__FSEXT_Function *);
  void          *(*__FSEXT_get_data) (int);
  void          *(*__FSEXT_set_data) (int, void *);
  int            (*__FSEXT_set_function) (int, __FSEXT_Function *);
  void           (*__dj_assert) (const char *, const char *, int);
  void           (*__dj_movedata) (void);
  int            (*__djgpp_set_ctrl_c) (int);
  int            (*__dpmi_get_real_mode_interrupt_vector) (int, __dpmi_raddr *);
  int            (*__dpmi_get_segment_base_address) (int, unsigned long *);
  unsigned long  (*__dpmi_get_segment_limit) (int);
  int            (*__dpmi_int) (int, __dpmi_regs *);
  int            (*__dpmi_unlock_linear_region) (__dpmi_meminfo *);
  void           (*__dpmi_yield) (void);
  void           (*__movedata) (unsigned, unsigned, unsigned, unsigned, size_t);
  void           (*_dos_getdate) (struct _dosdate_t*);
  int            (*_close) (int);
  void           (*_exit) (int);
  unsigned short (*_get_dos_version) (int);
  int            (*_go32_dpmi_allocate_dos_memory) (_go32_dpmi_seginfo *);
  int            (*_go32_dpmi_allocate_real_mode_callback_retf) (_go32_dpmi_seginfo *, _go32_dpmi_registers *);
  int            (*_go32_dpmi_free_dos_memory) (_go32_dpmi_seginfo *);
  int            (*_go32_dpmi_free_real_mode_callback) (_go32_dpmi_seginfo *);
  int            (*_go32_dpmi_lock_code) (void *, unsigned long);
  int            (*_go32_dpmi_lock_data) (void *, unsigned long);
  void           (*_go32_want_ctrl_break) (int);
  ssize_t        (*_write) (int, const void *, size_t);
  int            (*_chmod) (const char *, int, ...);
  int            (*atexit) (void (*func)(void));
  int            (*atoi) (const char *);
  long           (*atol) (const char *);
  void          *(*bsearch) (const void *, const void *, size_t, size_t, int (*)(const void *, const void *));
  void          *(*calloc) (size_t, size_t);
  int            (*creat) (const char *, mode_t);
  char          *(*ctime) (const time_t *);
  void           (*dosmemget) (unsigned long, size_t, void *);
  void           (*exit) (int);
  int            (*fclose) (FILE *);
  int            (*feof) (FILE *);
  int            (*ferror) (FILE *);
  int            (*fflush) (FILE *);
  char          *(*fgets) (char *, int, FILE *);
  int            (*fileno) (FILE *);
  FILE          *(*fopen) (const char *, const char *);
  int            (*fprintf) (FILE *, const char *, ...);
  int            (*fputc) (int, FILE *);
  int            (*fputs) (const char *, FILE *);
  size_t         (*fread) (void *, size_t, size_t, FILE *);
  void           (*free) (void *);
  size_t         (*fwrite) (const void *, size_t, size_t, FILE *);
  int            (*getc) (FILE *);
  int            (*getcbrk) (void);
  char          *(*getenv) (const char *);
  pid_t          (*getpid) (void);
  int            (*getrlimit) (int, struct rlimit *);
  int            (*gettimeofday) (struct timeval *, struct timezone *);
  int            (*isatty) (int);
  char          *(*itoa) (int, char *, int);
  int            (*kbhit) (void);
  struct tm     *(*localtime) (const time_t *);
  void           (*longjmp) (jmp_buf, int);
  void          *(*malloc) (size_t);
  void          *(*memchr) (const void *, int, size_t);
  void          *(*memcpy) (void *, const void *, size_t);
  void          *(*_memmove) (void *, const void *, size_t);
  void          *(*memset) (void *, int, size_t);
  time_t         (*mktime) (struct tm *);
  int            (*printf) (const char *, ...);
  void           (*psignal) (int, const char *);
  int            (*putc) (int, FILE *);
  int            (*puts) (const char *);
  void           (*qsort) (void *, size_t, size_t, int (*)(const void *, const void *));
  int            (*raise) (int);
  int            (*rand) (void);
  void          *(*realloc) (void *, size_t);
  void           (*rewind) (FILE *);
  void           (*setbuf) (FILE *, char *);
  int            (*setitimer) (int, struct itimerval *, struct itimerval *);
  int            (*setjmp) (jmp_buf);
  int            (*setrlimit) (int, const struct rlimit *);
  void         (*(*signal) (int, void (*)(int)))(int);
  unsigned int   (*sleep) (unsigned int);
  int            (*sprintf) (char *, const char *, ...);
  int            (*snprintf) (char *, size_t, const char *, ...);
  void           (*srand) (unsigned);
  int            (*sscanf) (const char *, const char *, ...);
  char          *(*strcat) (char *, const char *);
  char          *(*strchr) (const char *, int);
  int            (*strcmp) (const char *, const char *);
  char          *(*strcpy) (char *, const char *);
  char          *(*strdup) (const char *);
  int            (*stricmp) (const char *, const char *);
  size_t         (*strlen) (const char *);
  char          *(*strlwr) (char *);
  char          *(*strncat) (char *, const char *, size_t);
  int            (*strncmp) (const char *, const char *, size_t);
  char          *(*strncpy) (char *, const char *, size_t);
  int            (*strnicmp) (const char *, const char *, size_t);
  char          *(*strrchr) (const char *, int);
  char          *(*strstr) (const char *, const char *);
  char          *(*strtok) (char *, const char *);
  char          *(*strupr) (char *);
  time_t         (*time) (time_t *);
  int            (*unlink) (const char *);
  int            (*vfprintf) (FILE *, const char *, va_list);
  int            (*vsprintf) (char *, const char *, va_list);
  int            (*vsnprintf) (char *, size_t, va_list);
  int            (*vsscanf) (const char *, const char *, va_list);


  /* Put all variables AFTER the functions. This makes it easy to
   * catch uninitialized function pointers.
   */
  FILE              __dj_stdin, __dj_stdout, __dj_stderr;
  char            **__crt0_argv;
  unsigned short    __dj_ctype_flags  [256];
  unsigned short    __dj_ctype_tolower[256];
  unsigned short    __dj_ctype_toupper[256];
  jmp_buf          *__djgpp_exception_state_ptr;
  unsigned short    __dpmi_error;
  int               _fmode;
  __Go32_Info_Block _go32_info_block;
  int               errno;
};


extern void __dj_movedata (void);
extern void __movedata (unsigned src_sel, unsigned src_ofs,
                        unsigned dst_sel, unsigned dst_ofs,
                        size_t len);


/* import/export structure is allocated in dxe_map.c (when making an DXE)
 * or in dllinit.c (when making the application)
 */
extern struct st_symbol_t import_export;

#if !defined(MAKE_IMPORT)

/* We have to do this, because there's also a struct stat
 * TODO?: do this for all #defines in this file.
 */
#define stat(FILE, STATBUF) import_export.stat(FILE, STATBUF)

#undef  movmem
#define movmem(s,d,n)  import_export._memmove (d,s,n)

#if 0
  #undef  assert
  #define assert(test) ((void)((test) || (__dj_assert(#test,__FILE__,__LINE__),0)))
#endif

#define __FSEXT_alloc_fd                            import_export.__FSEXT_alloc_fd
#define __FSEXT_get_data                            import_export.__FSEXT_get_data
#define __FSEXT_set_data                            import_export.__FSEXT_set_data
#define __FSEXT_set_function                        import_export.__FSEXT_set_function
#define __crt0_argv                                 import_export.__crt0_argv
#define __dj_assert                                 import_export.__dj_assert
#define __dj_ctype_flags                            import_export.__dj_ctype_flags
#define __dj_ctype_tolower                          import_export.__dj_ctype_tolower
#define __dj_ctype_toupper                          import_export.__dj_ctype_toupper
#define __dj_movedata                               import_export.__dj_movedata
#define __dj_stderr                                 import_export.__dj_stderr
#define __dj_stdin                                  import_export.__dj_stdin
#define __dj_stdout                                 import_export.__dj_stdout
#define __djgpp_exception_state_ptr                 import_export.__djgpp_exception_state_ptr
#define __djgpp_set_ctrl_c                          import_export.__djgpp_set_ctrl_c
#define __dpmi_error                                import_export.__dpmi_error
#define __dpmi_get_real_mode_interrupt_vector       import_export.__dpmi_get_real_mode_interrupt_vector
#define __dpmi_get_segment_base_address             import_export.__dpmi_get_segment_base_address
#define __dpmi_get_segment_limit                    import_export.__dpmi_get_segment_limit
#define __dpmi_int                                  import_export.__dpmi_int
#define __dpmi_unlock_linear_region                 import_export.__dpmi_unlock_linear_region
#define __dpmi_yield                                import_export.__dpmi_yield
#define __movedata                                  import_export.__movedata
#define __udivdi3                                   import_export.__udivdi3
#define __umoddi3                                   import_export.__umoddi3
#define _close                                      import_export._close
#define _dos_getdate                                import_export._dos_getdate
#define _exit                                       import_export._exit
#define _fmode                                      import_export._fmode
#define _get_dos_version                            import_export._get_dos_version
#define _go32_dpmi_allocate_dos_memory              import_export._go32_dpmi_allocate_dos_memory
#define _go32_dpmi_allocate_real_mode_callback_retf import_export._go32_dpmi_allocate_real_mode_callback_retf
#define _go32_dpmi_free_dos_memory                  import_export._go32_dpmi_free_dos_memory
#define _go32_dpmi_free_real_mode_callback          import_export._go32_dpmi_free_real_mode_callback
#define _go32_dpmi_lock_code                        import_export._go32_dpmi_lock_code
#define _go32_dpmi_lock_data                        import_export._go32_dpmi_lock_data
#define _go32_info_block                            import_export._go32_info_block
#define _go32_want_ctrl_break                       import_export._go32_want_ctrl_break
#define _write                                      import_export._write
#define _chmod                                      import_export._chmod
#define atexit                                      import_export.atexit
#define atoi                                        import_export.atoi
#define atol                                        import_export.atol
#define bsearch                                     import_export.bsearch
#define calloc                                      import_export.calloc
#define creat                                       import_export.creat
#define ctime                                       import_export.ctime
#define dosmemget                                   import_export.dosmemget
#define errno                                       import_export.errno
#define exit                                        import_export.exit
#define fclose                                      import_export.fclose
#define feof                                        import_export.feof
#define ferror                                      import_export.ferror
#define fflush                                      import_export.fflush
#define fgets                                       import_export.fgets
#define fileno                                      import_export.fileno
#define fopen                                       import_export.fopen
#define fprintf                                     import_export.fprintf
#define fputc                                       import_export.fputc
#define fputs                                       import_export.fputs
#define fread                                       import_export.fread
#define free                                        import_export.free
#define fwrite                                      import_export.fwrite
#define getc                                        import_export.getc
#define getcbrk                                     import_export.getcbrk
#define getenv                                      import_export.getenv
#define getpid                                      import_export.getpid
#define getrlimit                                   import_export.getrlimit
#define gettimeofday                                import_export.gettimeofday
#define isatty                                      import_export.isatty
#define itoa                                        import_export.itoa
#define kbhit                                       import_export.kbhit
#define localtime                                   import_export.localtime
#define longjmp                                     import_export.longjmp
#define malloc                                      import_export.malloc
#define memchr                                      import_export.memchr
#define memcpy                                      import_export.memcpy
#define memmove                                     import_export._memmove
#define memset                                      import_export.memset
#define mktime                                      import_export.mktime
#define printf                                      import_export.printf
#define psignal                                     import_export.psignal
#define putc                                        import_export.putc
#define puts                                        import_export.puts
#define qsort                                       import_export.qsort
#define raise                                       import_export.raise
#define rand                                        import_export.rand
#define realloc                                     import_export.realloc
#define rewind                                      import_export.rewind
#define setbuf                                      import_export.setbuf
#define setitimer                                   import_export.setitimer
#define setjmp                                      import_export.setjmp
#define setrlimit                                   import_export.setrlimit
#define signal                                      import_export.signal
#define sleep                                       import_export.sleep
#define sprintf                                     import_export.sprintf
#define srand                                       import_export.srand
#define sscanf                                      import_export.sscanf
#define strcat                                      import_export.strcat
#define strchr                                      import_export.strchr
#define strcmp                                      import_export.strcmp
#define strcpy                                      import_export.strcpy
#define strdup                                      import_export.strdup
#define stricmp                                     import_export.stricmp
#define strlen                                      import_export.strlen
#define strlwr                                      import_export.strlwr
#define strncat                                     import_export.strncat
#define strncmp                                     import_export.strncmp
#define strncpy                                     import_export.strncpy
#define strnicmp                                    import_export.strnicmp
#define strrchr                                     import_export.strrchr
#define strstr                                      import_export.strstr
#define strtok                                      import_export.strtok
#define strupr                                      import_export.strupr
#define time                                        import_export.time
#define unlink                                      import_export.unlink
#define vfprintf                                    import_export.vfprintf
#define vsprintf                                    import_export.vsprintf
#define vsscanf                                     import_export.vsscanf

#endif  /* MAKE_IMPORT */
#endif  /* _w32_DXE_SYM_H */

