skinsdir=$(prefix)/share/xonclock/skins
skins_DATA = images/xonclock.png images/xonclock-a.png images/chrome-260.png images/chrome-128.png images/chrome-128-blue-out.png images/chrome-128-blue.png images/chrome-128-brown.png images/chrome-128-bs.png images/chrome-128-green.png

AM_CPPFLAGS=$(CPPFLAGS) -DOWN_SKIN=\"$(skinsdir)/xonclock.png\"

bin_PROGRAMS = xonclock
xonclock_SOURCES = 	\
	xonclock.c	\
	xonclock.h	\
	placement.c	\
	cache.c		\
	cache.h		\
	parse.c		\
	parse.h		\
	placement.h	\
	image.c		\
	image.h		\
	pmdesktop.c \
	pmdesktop.h \
	loaders/png.c	\
	loaders/png.h	\
	loaders/jpeg.c	\
	loaders/jpeg.h	\
	loaders/tiff.c	\
	loaders/tiff.h
