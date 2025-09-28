/*
 * This file is part of the FDNPKG project.
 * Copyright (C) 2013-2016 Mateusz Viste
 */

#include <stdlib.h>    /* NULL */
#include <stdio.h>     /* printf(), FILE, fclose()... */

#include "crc32.h"
#include "inf.h"       /* DEFLATE support */
#include "libgz.h"     /* include self for control */
#include "version.h"

#define GZ_FLAG_ASCII 1
#define GZ_FLAG_MULTIPART_CONTINUTATION 2
#define GZ_FLAG_EXTRA_FIELD_PRESENT 4
#define GZ_FLAG_ORIG_FILENAME_PRESENT 8
#define GZ_FLAG_FILE_COMMENT_PRESENT 16
#define GZ_FLAG_FILE_IS_ENCRYPTED 32


#define UNGZ_RETURN(x) \
free(buff); \
if (fd != NULL) fclose(fd); \
if (dstfd != NULL) fclose(dstfd); \
return(x);

/* gunzips a 'srcfile' file into 'destfile'. returns 0 on success, non-zero otherwise. */
int ungz(char *srcfile, char *destfile) {
  #define buffsize 32 * 1024l   /* must be at least 32K */
  unsigned char compmethod;
  unsigned long cksum, cksum_from_gz;
  FILE *fd = NULL;              /* I allocate all pointers to NULL here     */
  FILE *dstfd = NULL;           /* because UNGZ_RETURN() relies on this to  */
  unsigned char *buff = NULL;   /* know whether it needs to be freed or not */
  unsigned char flags;
  int extract_res;
  long filelen, compressedfilelen, gztotalfilelen;

  /* open the src file */
  fd = fopen(srcfile, "rb");
  if (fd == NULL) {
    UNGZ_RETURN(-3);
  }

  /* allocate buffers for data I/O */
  buff = malloc(buffsize);
  if (buff == NULL) {
    UNGZ_RETURN(-6);
  }

  /* read the uncompressed file length */
  fseek(fd, -4, SEEK_END); /* set the reading position inside the gz file */
  if (fread(buff, 4, 1, fd) != 1) {
    UNGZ_RETURN(-1);
  }
  filelen = buff[0] | buff[1] << 8 | buff[2] << 16 | buff[3] << 24;

  /* printf("Uncompressed file len is %d\n", filelen); */

  /* compute the total length of the gz file */
  gztotalfilelen = ftell(fd);

  /* printf("Total gz file len is %d\n", gztotalfilelen); */

  /* set the cursor of *fd to initial position */
  rewind(fd);

  /* check the magic header (2 bytes) - should be 0x1F 0x8B */
  if ((fread(buff, 2, 1, fd) != 1) || (buff[0] != 0x1F) || (buff[1] != 0x8B)) {
    UNGZ_RETURN(-7);
  }

  /* load the compression method (1 byte) - should be 0 (stored) or 8 (deflate) */
  if ((fread(&compmethod, 1, 1, fd) != 1) || ((compmethod != 0) && (compmethod != 8))) {
    UNGZ_RETURN(-8);
  }

  /* load flags (1 byte) */
  if (fread(&flags, 1, 1, fd) != 1) {
    UNGZ_RETURN(-9);
  }

  /* check that the file is not a continuation of a multipart gzip */
  if (flags & GZ_FLAG_MULTIPART_CONTINUTATION) {
    UNGZ_RETURN(-10);
  }

  /* check that the file is not encrypted */
  if (flags & GZ_FLAG_FILE_IS_ENCRYPTED) {
    UNGZ_RETURN(-11);
  }

  /* load (and discard) the file modification timestamp (4 bytes), the extra flags (1 byte) as well as OS type (1 byte) */
  if (fread(buff, 6, 1, fd) != 1) {
    UNGZ_RETURN(-12);
  }

  /* skip the extra field (if present) */
  if (flags & GZ_FLAG_EXTRA_FIELD_PRESENT) {
    int extrafieldlen;
    /* load the length of the extra field (2 bytes) */
    if (fread(buff, 2, 1, fd) != 1) {
      UNGZ_RETURN(-13);
    }
    extrafieldlen = buff[1];
    extrafieldlen <<= 8;
    extrafieldlen |= buff[0];
    /* skip the extra field */
    if (fread(buff, extrafieldlen, 1, fd) != 1) {
      UNGZ_RETURN(-14);
    }
  }
  /* skip the filename, if present (null terminated string) */
  if (flags & GZ_FLAG_ORIG_FILENAME_PRESENT) {
    int x;
    for (x = 0;; x++) {
      if ((fread(buff, 1, 1, fd) != 1) || (x > 1024)) {
        UNGZ_RETURN(-15);
      }
      if (buff[0] == 0) break;
    }
  }
  /* skip the file comment, if present (null terminated string) */
  if (flags & GZ_FLAG_FILE_COMMENT_PRESENT) {
    int x;
    for (x = 0;; x++) {
      if ((fread(buff, 1, 1, fd) != 1) || (x > 1024)) {
        UNGZ_RETURN(-16);
      }
      if (buff[0] == 0) break;
    }
  }

  /* compute the length of the compressed stream */
  compressedfilelen = gztotalfilelen - (ftell(fd) + 8);

  /* printf("compressed stream length is %d\n", compressedfilelen); */

  /* open the dst file */
  dstfd = fopen(destfile, "wb");
  if (dstfd == NULL) {  /* failed to open the dst file */
    UNGZ_RETURN(-17);
  }

  /* start reading and uncompressing the compressed data, computing CRC32 at the same time */
  cksum = crc32_init(); /* init the crc32 */
  extract_res = 0;   /* assume we will succeed */
  if (compmethod == 0) { /* if the file is stored, copy it over */
    unsigned long i, toread;
    for (i = 0; (signed)i < filelen;) {
      toread = filelen - i;
      if (toread > buffsize) toread = buffsize;
      if (fread(buff, toread, 1, fd) != 1) extract_res = -3;   /* read a chunk of data */
      crc32_feed(&cksum, buff, toread); /* update the crc32 checksum */
      if (fwrite(buff, toread, 1, fd) != 1) extract_res = -4; /* write data chunk to dst file */
      i += toread;
    }
  } else if (compmethod == 8) { /* the file is deflated */
    if (inf(fd, dstfd, buff, &cksum, compressedfilelen) != 0) {
      extract_res = -18;
    }
  } else { /* unknown compression scheme */
    extract_res = -19;
  }

  if (extract_res != 0) {
    UNGZ_RETURN(-20);
  }

  /* read the CRC32 */
  if (fread(buff, 4, 1, fd) != 1) {
    UNGZ_RETURN(-21);
  }
  cksum_from_gz = buff[0] | buff[1] << 8 | buff[2] << 16 | buff[3] << 24;

  /* compute the final (real) CRC */
  crc32_finish(&cksum);

  /* check the CRC validity */
  if (cksum != cksum_from_gz) {
    UNGZ_RETURN(-30);
  }

  UNGZ_RETURN(0);
}
