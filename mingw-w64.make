#
# GNU Makefile for the Windows based loader, cross compiled on Mac or Linux.
# Toolchain: Mingw-w64
#
# Author: Markus Thielen, mt@thiguten.de
#

# Check environment; setmac.sh must have been called
ifeq ($(TARGET_ARCH),)
$(error Please run ". ./setmac.sh" before building.)
endif

# --- setup make environment --------------------------------------------------

ARCH ?= $(TARGET_ARCH)-

CXX = $(ARCH)g++
CC = $(ARCH)gcc
AS = $(ARCH)as
AR = $(ARCH)gcc-ar
RC = $(ARCH)windres
srcdir = examples/
BUILDDIR = Release/

# --- build flags -------------------------------------------------------------

OPT ?= -Wall -Werror -O2
WARNINGS = -Wall
CFLAGS = $(OPT)
INCLUDES = -I. -I.. -I$(WINDOWS_TOOLDIR)/include
LDFLAGS_REL := -L./Release -lcef -Wl,-subsystem,windows
LDFLAGS := -L./Release -lcef

TARGET = loader_win_msg_loop.exe
OBJS = $(BUILDDIR)loader_win_msg_loop.o

# ---- rules ------------------------------------------------------------------

$(BUILDDIR)%.o: $(srcdir)%.c
	mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(WARNINGS) $(INCLUDES) -c $< -o $@

$(BUILDDIR)%.res: $(srcdir)%.rc
	mkdir -p $(BUILDDIR)
	$(RC) -I$(srcdir) -i $< -o $@

Release/$(TARGET): $(OBJS) $(RESS)
	@echo --- LINKING
	$(CC) $(CFLAGS) $(WARNINGS) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f $(BUILDDIR)*

