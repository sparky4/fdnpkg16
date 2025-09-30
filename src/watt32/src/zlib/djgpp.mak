# Makefile for zlib in Watt-32
#
# HAND-EDITED TO TARGET DJGPP
# To cross-compile on Linux:
#  make -f djgpp.mak CROSS=1

SOURCE = adler32.c compress.c crc32.c gzio.c uncompr.c deflate.c \
         trees.c zutil.c inflate.c infback.c inftrees.c inffast.c

OBJS = $(OBJDIR)\adler32.obj  $(OBJDIR)\compress.obj \
       $(OBJDIR)\crc32.obj    $(OBJDIR)\gzio.obj     \
       $(OBJDIR)\uncompr.obj  $(OBJDIR)\deflate.obj  \
       $(OBJDIR)\trees.obj    $(OBJDIR)\zutil.obj    \
       $(OBJDIR)\inflate.obj  $(OBJDIR)\infback.obj  \
       $(OBJDIR)\inftrees.obj $(OBJDIR)\inffast.obj

########################################################################

ifeq ($(CROSS),)
TOOLCHAIN_PREFIX=
else
TOOLCHAIN_PREFIX=i586-pc-msdosdjgpp-
endif

CC = $(TOOLCHAIN_PREFIX)gcc
AS = $(TOOLCHAIN_PREFIX)as
AR = $(TOOLCHAIN_PREFIX)ar
RANLIB = $(TOOLCHAIN_PREFIX)ranlib

CFLAGS = -O2 -W -Wall -I../../inc
# -g -gcoff

OBJDIR = ../djgpp

OBJS := $(subst \,/,$(OBJS))
OBJS := $(OBJS:.obj=.o)

all: $(OBJS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJS)

-include ../djgpp/zlib.dep

