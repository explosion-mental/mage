# mage version
VERSION = 0.8

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# freetype
FREETYPELIBS = -lfontconfig -lXft
FREETYPEINC = /usr/include/freetype2

# includes and libs
INCS = -I. -I/usr/include -I/usr/include/freetype2 -I${X11INC}
LIBS = -L/usr/lib -lc -lm -L${X11LIB} -lXft -lfontconfig -lX11 -lImlib2

# OpenBSD (uncomment)
#INCS = -I. -I${X11INC} -I${X11INC}/freetype2

# FreeBSD (uncomment)
#INCS = -I. -I/usr/local/include -I/usr/local/include/freetype2 -I${X11INC}
#LIBS = -L/usr/local/lib -lc -lm -L${X11LIB} -lXft -lfontconfig -lX11

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -DVERSION=\"${VERSION}\" -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700
CFLAGS  = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}
#CFLAGS  = -g -std=c99 -pedantic -Wall -Wextra -O3 ${INCS} ${CPPFLAGS} -flto -fsanitize=address,undefined,leak
LDFLAGS = ${LIBS}
#LDFLAGS = -g ${LIBS} ${CFLAGS}

# compiler and linker
CC = cc
