# OpenWatcom tests makefile for Glide3
# This makefile MUST be processed by GNU make!!!
#
#  Copyright (c) 2004 - Borca Daniel
#  Email : dborca@users.sourceforge.net
#  Web   : http://www.geocities.com/dborca
#

#
#  Available options:
#
#    Environment variables:
#	CPU		optimize for the given processor.
#			default = 5s (Pentium, stack)
#	DEBUG=1		disable optimizations and build for debug.
#			default = no
#
#    Targets:
#	<file.exe>	build a specific file
#

.PHONY: all
.SUFFIXES: .c .obj .exe
.SECONDARY: tlib.obj fxos.obj

FX_GLIDE_HW = h5
TOP = ../../..

CC = wcl386
CFLAGS = -wx
CFLAGS += -I$(TOP)/$(FX_GLIDE_HW)/glide3/src -I$(TOP)/$(FX_GLIDE_HW)/incsrc
CFLAGS += -I$(TOP)/swlibs/fxmisc
CFLAGS += -D__DOS__ -DH3
CFLAGS += -D__DOS32__

ifdef DEBUG
CFLAGS += -od -d2
else
CPU ?= 5s
CFLAGS += -ox -$(CPU)
endif

LDFLAGS = -k32768

LDLIBS = $(TOP)/$(FX_GLIDE_HW)/lib/glide3x.lib

# Watcom woes: pass parameters through environment vars
export WCC386 = $(subst /,\,$(CFLAGS))
export WCL386 = -zq

.c.obj:
	$(CC) -fo=$@ -c $<
%.exe: tlib.obj %.obj
	$(CC) -fe=$@ $(LDFLAGS) $^ $(subst /,\,$(LDLIBS))

all:
	$(error Must specify <filename.exe> to build)

sbench.exe: sbench.obj fxos.obj tlib.obj
	$(CC) -fe=$@ $(LDFLAGS) $^ $(subst /,\,$(LDLIBS))

fxos.obj: $(TOP)/swlibs/fxmisc/fxos.c
	$(CC) -fo=$@ -c $<
