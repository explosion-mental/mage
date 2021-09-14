# mage - iMAGE viewer
# See LICENSE file for copyright and license details.

include config.mk

SRC = mage.c drw.c util.c
OBJ = ${SRC:.c=.o}

all: options mage

options:
	@echo mage build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

config.h:
	cp config.def.h config.h

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

mage: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f mage ${OBJ} mage-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p mage-${VERSION}
	@cp -R LICENSE Makefile config.mk config.def.h ${SRC} mage-${VERSION}
	@tar -cf mage-${VERSION}.tar mage-${VERSION}
	@gzip mage-${VERSION}.tar
	@rm -rf mage-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f mage ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/mage
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1

#@mkdir -p ${DESTDIR}${MANPREFIX}/man1
#@cp mage.1 ${DESTDIR}${MANPREFIX}/man1/mage.1
#@chmod 644 ${DESTDIR}${MANPREFIX}/man1/mage.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/mage

.PHONY: all options clean dist install uninstall
