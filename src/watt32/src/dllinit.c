/*
 * dllinit.c - DLL initialization code
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

#include <stdio.h>
#include <string.h>

#if defined(__DJGPP__) && defined(WATT32_DOS_DLL)

#define MAKE_DXE       /* signal that we include dxe_map.c from a DXE */
#include "wattcp.h"
#include "sock_ini.h"
#include "strings.h"
#include "misc.h"
#include "dxe_map.c"

#define DXE_DEBUG      1

#if (DXE_DEBUG)
  #define DXE_MSG(s)   outsnl (s)
#else
  #define DXE_MSG(s)   ((void)0)
#endif

static int   dxe_init (void);
static void *dxe_symbol (const char *symbol_name);

struct st_symbol_t import_export = { dxe_init, dxe_symbol };
static st_map_t   *symbol;

static int dxe_init (void)
{
  DXE_MSG ("dxe_init()");

  symbol = map_create (50);     /* Read comment in map.h! */
  symbol->cmp_key = (int (*)(const void*, const void*)) strcmp;

#if 0
  #include "djgpp/dxe/dxe_init.inc"
#else
  symbol = map_put (symbol, "sock_init", sock_init);
  symbol = map_put (symbol, "sock_exit", sock_exit);
  symbol = map_put (symbol, "wattcpVersion", wattcpVersion);
  symbol = map_put (symbol, "wattcpCapabilities", wattcpCapabilities);
#endif
  return (0);
}


/*
 * Normally, the code that uses a dynamic library knows what it wants,
 * i.e., it searches for specific symbol names. So, for a program that
 * uses a DXE the code could look something like:
 *
 * void *handle         = open_module (MODULE_NAME);
 * int (*function)(int) = ((strruct st_symbol_t*) handle)->function;
 *
 * However, by adding a symbol loading function, st_symbol_t doesn't have
 * to be updated if the DXE should export more or other symbols, which
 * makes using a DXE less error prone. Changing st_symbol_t would also
 * require a recompile of the code that uses the DXE (which is a *bad* thing).
 * A symbol loading function also makes an "extension API" a bit more elegant,
 * because the extension functions needn't be hardcoded in st_symbol_t;
 */
static void *dxe_symbol (const char *symbol_name)
{
  DXE_MSG (symbol_name);
  return map_get (symbol, symbol_name);
}

#endif  /* __DJGPP__ && WATT32_DOS_DLL */
