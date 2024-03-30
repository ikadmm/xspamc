#
#  XSpamc - Spamassassin filter for Xmail
#  Copyright (C) 2004 2005  Dario Jakopec
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#  Suggestions, patches and comments are welcome.
#  Dario Jakopec <dario@henry.it>
#
#  Release 0.4.4 - 05/08/2005
#
#
CC = gcc
LD = gcc
INCLUDE = -I.

CFLAGS = $(INCLUDE) -DUNIX -DLINUX
LDFLAGS =

OUTDIR = bin
SRCDIR = .

XSPAMC_TRGS = $(OUTDIR)/xspamc
XSPAMC_SRCS = $(SRCDIR)/dep.c $(SRCDIR)/libspamc.c $(SRCDIR)/utility.c $(SRCDIR)/xspamc.c
XSPAMC_OBJS = $(OUTDIR)/dep.o $(OUTDIR)/libspamc.o $(OUTDIR)/utility.o $(OUTDIR)/xspamc.o

ALL_TARGETS = $(XSPAMC_TRGS)
ALL_OBJECTS = $(XSPAMC_OBJS)
ALL_SOURCES = $(XSPAMC_SRCS)

$(OUTDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -o $(OUTDIR)/$*.o -c $(SRCDIR)/$*.c

all: $(OUTDIR) $(ALL_TARGETS)

$(XSPAMC_TRGS): $(XSPAMC_OBJS)
	$(LD) $(LDFLAGS) -o $(XSPAMC_TRGS) $(XSPAMC_OBJS)

$(OUTDIR):
	@mkdir $(OUTDIR)

distclean: clean
	@rm -rf $(OUTDIR)

clean:
	@rm -f $(ALL_TARGETS)
	@rm -f $(ALL_OBJECTS)
	@rm -f *~
