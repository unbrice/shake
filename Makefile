VERSION=0.27
CC	= gcc
LFLAGS	= -lattr

BINDIR	= $(DESTDIR)/usr/bin
MANDIR	= $(DESTDIR)/usr/share/man/man8

CFLAGS += -std=gnu99 -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64   \
	-D_XOPEN_SOURCE=600 -O2 -D_POSIX_C_SOURCE=200112L -Wall     \
	-pedantic-errors  -Wcast-align -Wpointer-arith	    	    \
	-Wbad-function-cast -DVERSION=\"${VERSION}\"
# CFLAGS += -O0 -ggdb3 -Wconversion -Werror # -pg
CFLAGS += -DNDEBUG

.PHONY: all clean dist install uninstall

all:    shake unattr
static: all shake-static
dist:
	rm -f *.o
	rm -f shake??????
	rm -f callgrind.out.*
doc: doc/shake.8 doc/unattr.8
clean:  dist
	rm -f unattr shake shake-static
	find -name '*.~' -exec rm {} \;

shake: executive.o judge.o linux.o main.o msg.o
	$(CC) $(CFLAGS) $(LFLAGS) $? -o $@

shake-static: executive.o judge.o linux.o main.o msg.o
	$(CC) $(CFLAGS) $(LFLAGS) -static -u attr_setf -u attr_getf $? -o $@

unattr: executive.o unattr.o linux.o
	$(CC) $(CFLAGS) $(LFLAGS) $? -o $@

%.o: %.c executive.h judge.h linux.h msg.h Makefile
	$(CC) $(CFLAGS) -c $< -o $@

doc/%.8: % Makefile \
	   doc/unattr-man_insert doc/shake-man_insert doc/fdl-man_insert
	help2man -s8 -N -i doc/fdl-man_insert -I doc/$<-man_insert \
		 ./$< -o doc/$<.8

install: all
	cp shake unattr $(BINDIR)
	cp doc/shake.8 doc/unattr.8 $(MANDIR)

uninstall:
	rm $(BINDIR)/{shake,unattr} -f
	rm $(MANDIR)/{shake,unattr}.8 -f

tarball: doc clean
	tar -C.. -cj shake -f ../shake-${VERSION}.tar.bz2 \
	--exclude CVS --exclude semantic.cache 
