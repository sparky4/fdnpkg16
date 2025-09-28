/*
 * debugging module for FDNPKG, to track fopen() vs. fclose() calls, as well
 * as malloc/calloc() vs free() calls.
 *
 * Copyright (C) 2015-2016 Mateusz Viste
 */

#ifndef DBG_H_SENTINEL
#define DBG_H_SENTINEL

/* file debugging */
FILE *dbg_fopen(char *fname, char *mode);
int dbg_fclose(FILE *fd);

/* mem debugging */
void *dbg_malloc(long sz);
void *dbg_calloc(long sz, long n);
void dbg_free(void *ptr);

/* stats */
void dbg_printstats(void);

#define fopen(x,y) dbg_fopen(x,y)
#define fclose(x) dbg_fclose(x)
#define malloc(x) dbg_malloc(x)
#define calloc(x,y) dbg_calloc(x,y)
#define free(x) dbg_free(x)

#endif
