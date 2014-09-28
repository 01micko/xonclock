# -*- Autoconf -*- ---------------------------------------------------
#
# xonclock autoconf script
# Process this file with autoconf to produce a configure script.
#
# --------------------------------------------------------------------

# --------------------------------------------------------------------
# initial checks
# --------------------------------------------------------------------
AC_PREREQ(2.58)
AC_INIT(xonclock, 0.0.9.3, cyrill_grk@users.sourceforge.net)
AM_INIT_AUTOMAKE(@PACKAGE_NAME@, @PACKAGE_VERSION@)
AC_MSG_CHECKING([for package sources existence])
AC_CONFIG_SRCDIR([src/xonclock.c])
AC_CONFIG_SRCDIR([src/xonclock.h])
AC_CONFIG_SRCDIR([src/placement.c])
AC_CONFIG_SRCDIR([src/placement.h])
AC_CONFIG_SRCDIR([src/cache.c])
AC_CONFIG_SRCDIR([src/cache.h])
AC_CONFIG_SRCDIR([src/image.c])
AC_CONFIG_SRCDIR([src/image.h])
AC_CONFIG_SRCDIR([src/pmdesktop.c])
AC_CONFIG_SRCDIR([src/pmdesktop.h])
AC_CONFIG_SRCDIR([src/loaders/png.c])
AC_CONFIG_SRCDIR([src/loaders/png.h])
AC_CONFIG_SRCDIR([src/loaders/jpeg.c])
AC_CONFIG_SRCDIR([src/loaders/jpeg.h])
AC_CONFIG_SRCDIR([src/loaders/tiff.c])
AC_CONFIG_SRCDIR([src/loaders/tiff.h])
AC_CONFIG_SRCDIR([src/images/xonclock.png])
AC_CONFIG_SRCDIR([src/images/xonclock-a.png])
AC_CONFIG_SRCDIR([src/images/chrome-128-blue-out.png])
AC_CONFIG_SRCDIR([src/images/chrome-128-bs.png])
AC_CONFIG_SRCDIR([src/images/chrome-260.png])
AC_CONFIG_SRCDIR([src/images/chrome-128-blue.png])
AC_CONFIG_SRCDIR([src/images/chrome-128-green.png])
AC_CONFIG_SRCDIR([src/images/chrome-128-brown.png])
AC_CONFIG_SRCDIR([src/images/chrome-128.png])
AC_CONFIG_SRCDIR([man/xonclock.1])
AC_CONFIG_SRCDIR([src/xonclockrc-example])
AC_MSG_RESULT([ok])

# --------------------------------------------------------------------
# add our C flags
# --------------------------------------------------------------------
CFLAGS="-O2 -Wall -pipe $CFLAGS"

# --------------------------------------------------------------------
# checks for GNU programs
# --------------------------------------------------------------------
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for GNU programs and ANSI C header files])
AC_PROG_CC(gcc)
AC_GNU_SOURCE
AC_C_CONST
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_LANG([C])

# --------------------------------------------------------------------
# package name and version
# --------------------------------------------------------------------
AC_SUBST([NAME], [@PACKAGE_NAME@])
AC_SUBST([VERSION], [@PACKAGE_VERSION@])

# --------------------------------------------------------------------
# check for typedefs, structures and compiler characteristics
# --------------------------------------------------------------------
AC_C_CONST

# --------------------------------------------------------------------
# check for ANSI C header files
# --------------------------------------------------------------------
AC_HEADER_STDC

# --------------------------------------------------------------------
# standart X Window paths
# --------------------------------------------------------------------
AC_PATH_XTRA

# --------------------------------------------------------------------
# additional features
# --------------------------------------------------------------------
#AC_ARG_ENABLE([xo_debug],
#	[  --enable-xo-debug       turns on debug messages],
#	[AC_DEFINE(XO_DEBUG)],[])

# --------------------------------------------------------------------
# header checking without -DFUNCNAME
# --------------------------------------------------------------------
AC_DEFUN([XO_CHECK_HEADER],
[(
	AC_CHECK_HEADER([$1], AC_SUBST([HAVE_$2],),
	[AC_MSG_ERROR([unable to find $1])])
)])

# --------------------------------------------------------------------
# checks for particular headers
# --------------------------------------------------------------------
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for particular header files])
XO_CHECK_HEADER([errno.h],[ERRNO_H])
XO_CHECK_HEADER([getopt.h],[GETOPT_H])
XO_CHECK_HEADER([limits.h],[LIMITS_H])
XO_CHECK_HEADER([math.h],[MATH_H])
XO_CHECK_HEADER([signal.h],[SIGNAL_H])
XO_CHECK_HEADER([stdarg.h],[STDARG_H])
XO_CHECK_HEADER([stdio.h],[STDIO_H])
XO_CHECK_HEADER([stdlib.h],[STDLIB_H])
XO_CHECK_HEADER([string.h],[STRING_H])
XO_CHECK_HEADER([sys/time.h],[SYS_TIME_H])
XO_CHECK_HEADER([time.h],[TIME_H])
XO_CHECK_HEADER([unistd.h],[UNISTD_H])
XO_CHECK_HEADER([sys/types.h],[SYS_TYPES_H])
XO_CHECK_HEADER([pwd.h],[PWD_H])
XO_CHECK_HEADER([errno.h],[ERRNO_H])
XO_CHECK_HEADER([pthread.h],[PTHREAD_H])

# --------------------------------------------------------------------
# Xft config
# --------------------------------------------------------------------
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for Xft configuration])
AC_CHECK_PROG([XO_XFT_CFLAGS],[xft-config],[`xft-config --cflags`],
	[AC_MSG_ERROR([unable to find xft-config])])
AC_CHECK_PROG([XO_XFT_LIBS],[xft-config],[`xft-config --libs`],
	[AC_MSG_ERROR([unable to find xft-config])])


# --------------------------------------------------------------------
# libpng config
# --------------------------------------------------------------------
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for libpng configuration])
AC_CHECK_PROG([XO_PNG_CFLAGS],[libpng-config],[`libpng-config --cflags`],
	[AC_MSG_ERROR([unable to find libpng-config])])
AC_CHECK_PROG([XO_PNG_LIBS],[libpng-config],[`libpng-config --libs`],
	[AC_MSG_ERROR([unable to find libpng-config])])


# --------------------------------------------------------------------
# Make unique items in built options
# --------------------------------------------------------------------
dnl
dnl
AC_DEFUN([XO_TMP], [tmpx_$1])
AC_DEFUN([XO_UNIQ],
[(\
	echo -n '' > XO_TMP($1)
	xo_buf=""
	for i in $$1; do
	    j=`echo $i | sed -e 's/^-.*//g'`
	case $j in
	''[)] # option
		if [[ ! -z "$xo_buf" ]]; then
		   echo $xo_buf >> XO_TMP($1)
		fi
		xo_buf="$i"
		;;
	*[)] # suboption
		xo_buf="$xo_buf $i"
		;;
	esac
	done
	if [[ ! -z "$xo_buf" ]]; then
	   echo $xo_buf >> XO_TMP($1)
	fi
	echo -n `cat XO_TMP($1) | sort -b | uniq` > XO_TMP($1)
)
$1=`cat XO_TMP($1)`
rm -f XO_TMP($1)
])
dnl
dnl
AC_MSG_NOTICE([])
AC_MSG_CHECKING([--> Making unique items of build options])
dnl
dnl concateinate options
CPPFLAGS="$CPPFLAGS $X_CFLAGS $XO_XFT_CFLAGS $XO_PNG_CFLAGS"
LDFLAGS="$LDFLAGS $X_LIBS $XO_XFT_LIBS $XO_PNG_LIBS -ltiff -ljpeg -lXpm"
LIBS="$LIBS -lm -lXrender"
dnl
dnl and process them
dnl
XO_UNIQ(CFLAGS)
XO_UNIQ(CPPFLAGS)
XO_UNIQ(LDFLAGS)
XO_UNIQ(LIBS)
dnl
dnl OK --->
AC_MSG_RESULT([ok])


# --------------------------------------------------------------------
# checking for headers
# --------------------------------------------------------------------
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for X11, Xft, Xpm, libpng, libtiff, libjpeg header files])

dnl
dnl checks for X11, Xft, Xpm headers
dnl
XO_CHECK_HEADER([X11/Xatom.h],[HAVE_X11_XATOM_H])
XO_CHECK_HEADER([X11/Xft/Xft.h],[HAVE_X11_XFT_XFT_H])
XO_CHECK_HEADER([X11/Xos.h],[HAVE_X11_XOS_H])
XO_CHECK_HEADER([X11/Xlib.h],[HAVE_X11_XLIB_H])
XO_CHECK_HEADER([X11/Xutil.h],[HAVE_X11_XUTIL_H])
XO_CHECK_HEADER([X11/extensions/Xrender.h],[HAVE_X11_EXTENSIONS_XRENDER_H])
XO_CHECK_HEADER([X11/extensions/shape.h],[HAVE_X11_EXTENSIONS_SHAPE_H])
XO_CHECK_HEADER([X11/xpm.h],[HAVE_X11_XPM_H])

dnl
dnl checks for PNG headers
dnl
XO_CHECK_HEADER([png.h],[HAVE_PNG_H])

dnl
dnl checks for JPEG headers
dnl
XO_CHECK_HEADER([jpeglib.h],[HAVE_JPEGLIB_H])

dnl
dnl checks for TIFF headers
dnl
XO_CHECK_HEADER([tiff.h],[HAVE_TIFF_H])


# --------------------------------------------------------------------
# checking for libraries
# --------------------------------------------------------------------
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for X11 library])
dnl
dnl X11 library
AC_CHECK_LIB([X11], [XOpenDisplay],,
	[AC_MSG_ERROR([unable to find XOpenDisplay in X11 library])])
AC_CHECK_LIB([Xext], [XShapeCombineMask],,
	[AC_MSG_ERROR([unable to find XShapeCombineMask in Xext library])])
dnl
dnl Xft library
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for Xft library])
AC_CHECK_LIB([Xft], [XftDrawCreate],,
	[AC_MSG_ERROR([unable to find XftDrawCreate in Xft library])])

dnl
dnl PNG library
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for PNG library])
AC_CHECK_LIB([png], [png_create_read_struct],,
	[AC_MSG_ERROR([unable to find png_create_read_struct in PNG library])])

dnl
dnl JPEG library
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for JPEG library])
AC_CHECK_LIB([jpeg], [jpeg_CreateDecompress],,
	[AC_MSG_ERROR([unable to find jpeg_CreateDecompress in JPEG library])])

dnl
dnl TIFF library
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for TIFF library])
AC_CHECK_LIB([tiff], [TIFFOpen],,
	[AC_MSG_ERROR([unable to find TIFFOpen in TIFF library])])

dnl
dnl Xpm library
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for XPM library])
AC_CHECK_LIB([Xpm], [XpmReadFileToImage],,
	[AC_MSG_ERROR([unable to find XpmReadFileToImage in XPM library])])

dnl
dnl
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for pthread library])
AC_CHECK_LIB([pthread], [pthread_create],,
	[AC_MSG_ERROR([unable to find pthread_create in pthread library])])



# --------------------------------------------------------------------
# functions checking without -DFUNCNAME
# --------------------------------------------------------------------
AC_DEFUN([XO_CHECK_FUNC],
[(
	AC_CHECK_FUNC([$1], AC_SUBST([HAVE_$2],),
	[AC_MSG_ERROR([unable to find $1])])
)])

# --------------------------------------------------------------------
# checking for functions
# --------------------------------------------------------------------
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for particular functions])
XO_CHECK_FUNC([atoi],[ATOI])
XO_CHECK_FUNC([calloc],[CALLOC])
XO_CHECK_FUNC([cosf],[COSF])
XO_CHECK_FUNC([exit],[EXIT])
XO_CHECK_FUNC([fflush],[FFLUSH])
XO_CHECK_FUNC([fprintf],[FPRINTF])
XO_CHECK_FUNC([free],[FREE])
XO_CHECK_FUNC([getopt_long],[GETOPT_LONG])
XO_CHECK_FUNC([localtime],[LOCALTIME])
XO_CHECK_FUNC([malloc],[MALLOC])
XO_CHECK_FUNC([memset],[MEMSET])
XO_CHECK_FUNC([printf],[PRINTF])
XO_CHECK_FUNC([setitimer],[SETITIMER])
XO_CHECK_FUNC([sigaction],[SIGACTION])
XO_CHECK_FUNC([sigemptyset],[SIGEMPTYSET])
XO_CHECK_FUNC([sinf],[SINF])
XO_CHECK_FUNC([sleep],[SLEEP])
XO_CHECK_FUNC([strcasecmp],[STRCASECMP])
XO_CHECK_FUNC([strcmp],[STRCMP])
XO_CHECK_FUNC([strerror],[STRERROR])
XO_CHECK_FUNC([strftime],[STRFTIME])
XO_CHECK_FUNC([strncmp],[STRNCMP])
XO_CHECK_FUNC([strtok],[STRTOK])
XO_CHECK_FUNC([time],[TIME])
XO_CHECK_FUNC([vfprintf],[VFPRINTF])
XO_CHECK_FUNC([abs],[ABS])
XO_CHECK_FUNC([fclose],[FCLOSE])
XO_CHECK_FUNC([fopen],[FOPEN])
XO_CHECK_FUNC([fseek],[FSEEK])
XO_CHECK_FUNC([ftell],[FTELL])
XO_CHECK_FUNC([getpid],[GETPID])
XO_CHECK_FUNC([kill],[KILL])
XO_CHECK_FUNC([localtime],[LOCALTIME])
XO_CHECK_FUNC([memcpy],[MEMCPY])
XO_CHECK_FUNC([rewind],[REWIND])
XO_CHECK_FUNC([setjmp],[SETJMP])
XO_CHECK_FUNC([strcat],[STRCAT])
XO_CHECK_FUNC([strlen],[STRLEN])
XO_CHECK_FUNC([getpwuid],[GETPWUID])
XO_CHECK_FUNC([getuid],[GETUID])
XO_CHECK_FUNC([nanosleep],[NANOSLEEP])
XO_CHECK_FUNC([usleep],[USLEEP])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for pthread functions])
XO_CHECK_FUNC([pthread_create],[PTHREAD_CREATE])
XO_CHECK_FUNC([pthread_join],[PTHREAD_JOIN])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for X11 functions])
XO_CHECK_FUNC([XCloseDisplay],[XCLOSEDISPLAY])
XO_CHECK_FUNC([XCopyArea],[XCOPYAREA])
XO_CHECK_FUNC([XCreateGC],[XCREATEGC])
XO_CHECK_FUNC([XCreatePixmap],[XCREATEPIXMAP])
XO_CHECK_FUNC([XCreateWindow],[XCREATEWINDOW])
XO_CHECK_FUNC([XDestroyImage],[XDESTROYIMAGE])
XO_CHECK_FUNC([XDestroyWindow],[XDESTROYWINDOW])
XO_CHECK_FUNC([XFlush],[XFLUSH])
XO_CHECK_FUNC([XFreeGC],[XFREEGC])
XO_CHECK_FUNC([XGetWindowAttributes],[XGETWINDOWATTRIBUTES])
XO_CHECK_FUNC([XLowerWindow],[XLOWERWINDOW])
XO_CHECK_FUNC([XMapWindow],[XMAPWINDOW])
XO_CHECK_FUNC([XNextEvent],[XNEXTEVENT])
XO_CHECK_FUNC([XRenderCompositeDoublePoly],[XRENDERCOMPOSITEDOUBLEPOLY])
XO_CHECK_FUNC([XRenderFindStandardFormat],[XRENDERFINDSTANDARDFORMAT])
XO_CHECK_FUNC([XRenderFreePicture],[XRENDERFREEPICTURE])
XO_CHECK_FUNC([XSelectInput],[XSELECTINPUT])
XO_CHECK_FUNC([XSendEvent],[XSENDEVENT])
XO_CHECK_FUNC([XSetWindowBackgroundPixmap],[XSETWINDOWBACKGROUNDPIXMAP])
XO_CHECK_FUNC([XShapeCombineMask],[XSHAPECOMBINEMASK])

XO_CHECK_FUNC([XAllocClassHint],[XALLOCCLASSHINT])
XO_CHECK_FUNC([XChangeProperty],[XCHANGEPROPERTY])
XO_CHECK_FUNC([XCloseDisplay],[XCLOSEDISPLAY])
XO_CHECK_FUNC([XCopyArea],[XCOPYAREA])
XO_CHECK_FUNC([XFreePixmap],[XFREEPIXMAP])
XO_CHECK_FUNC([XGetVisualInfo],[XGETVISUALINFO])
XO_CHECK_FUNC([XInternAtom],[XINTERNATOM])
XO_CHECK_FUNC([XMoveWindow],[XMOVEWINDOW])
XO_CHECK_FUNC([XOpenDisplay],[XOPENDISPLAY])
XO_CHECK_FUNC([XPutImage],[XPUTIMAGE])
XO_CHECK_FUNC([XRaiseWindow],[XRAISEWINDOW])
XO_CHECK_FUNC([XSetClassHint],[XSETCLASSHINT])
XO_CHECK_FUNC([XSetSelectionOwner],[XSETSELECTIONOWNER])
XO_CHECK_FUNC([XVisualIDFromVisual],[XVISUALIDFROMVISUAL])

XO_CHECK_FUNC([XftDrawCreate],[XFTDRAWCREATE])
XO_CHECK_FUNC([XftFontOpenName],[XFTFONTOPENNAME])

#--- there are macroses
#XO_CHECK_FUNC([DefaultColormap],[DEFAULTCOLORMAP])
#XO_CHECK_FUNC([DefaultDepth],[DEFAULTDEPTH])
#XO_CHECK_FUNC([DefaultGC],[DEFAULTGC])
#XO_CHECK_FUNC([DefaultScreen],[DEFAULTSCREEN])
#XO_CHECK_FUNC([DefaultVisual],[DEFAULTVISUAL])
#XO_CHECK_FUNC([DisplayHeight],[DISPLAYHEIGHT])
#XO_CHECK_FUNC([DisplayWidth],[DISPLAYWIDTH])
#XO_CHECK_FUNC([RootWindow],[ROOTWINDOW])
#XO_CHECK_FUNC([ScreenOfDisplay],[SCREENOFDISPLAY])

AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for Xft functions])
XO_CHECK_FUNC([XftColorAllocValue],[XFTCOLORALLOCVALUE])
XO_CHECK_FUNC([XftColorFree],[XFTCOLORFREE])
XO_CHECK_FUNC([XftDrawDestroy],[XFTDRAWDESTROY])
XO_CHECK_FUNC([XftDrawPicture],[XFTDRAWPICTURE])
XO_CHECK_FUNC([XftDrawSrcPicture],[XFTDRAWSRCPICTURE])
XO_CHECK_FUNC([XftDrawString8],[XFTDRAWSTRING8])
XO_CHECK_FUNC([XftFontClose],[XFTFONTCLOSE])
XO_CHECK_FUNC([XftTextExtents8],[XFTTEXTEXTENTS8])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for Xpm functions])
XO_CHECK_FUNC([XpmReadFileToImage],[XPMREADFILETOIMAGE])
XO_CHECK_FUNC([XpmReadFileToPixmap],[XPMREADFILETOPIXMAP])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for PNG functions])
XO_CHECK_FUNC([png_create_info_struct],[PNG_CREATE_INFO_STRUCT])
XO_CHECK_FUNC([png_create_read_struct],[PNG_CREATE_READ_STRUCT])
XO_CHECK_FUNC([png_destroy_read_struct],[PNG_DESTROY_READ_STRUCT])
XO_CHECK_FUNC([png_destroy_read_struct],[PNG_DESTROY_READ_STRUCT])
XO_CHECK_FUNC([png_get_IHDR],[PNG_GET_IHDR])
XO_CHECK_FUNC([png_init_io],[PNG_INIT_IO])
XO_CHECK_FUNC([png_read_image],[PNG_READ_IMAGE])
XO_CHECK_FUNC([png_read_info],[PNG_READ_INFO])
XO_CHECK_FUNC([png_set_expand],[PNG_SET_EXPAND])
XO_CHECK_FUNC([png_set_gray_to_rgb],[PNG_SET_GRAY_TO_RGB])
XO_CHECK_FUNC([png_set_packing],[PNG_SET_PACKING])
XO_CHECK_FUNC([png_set_strip_16],[PNG_SET_STRIP_16])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for TIFF functions])
XO_CHECK_FUNC([TIFFClose],[TIFFCLOSE])
XO_CHECK_FUNC([TIFFGetField],[TIFFGetField])
XO_CHECK_FUNC([TIFFOpen],[TIFFOpen])
XO_CHECK_FUNC([TIFFReadRGBAImageOriented],[TIFFREADRGBAIMAGEORIENTED])
XO_CHECK_FUNC([_TIFFfree],[_TIFFFREE])
XO_CHECK_FUNC([_TIFFmalloc],[_TIFFMALLOC])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--> checking for JPEG functions])
XO_CHECK_FUNC([jpeg_CreateDecompress],[JPEG_CREATEDECOMPRESS])
XO_CHECK_FUNC([jpeg_destroy_decompress],[JPEG_DESTROY_DECOMPRESS])
XO_CHECK_FUNC([jpeg_finish_decompress],[JPEG_FINISH_DECOMPRESS])
XO_CHECK_FUNC([jpeg_read_header],[JPEG_READ_HEADER])
XO_CHECK_FUNC([jpeg_read_scanlines],[JPEG_READ_SCANLINES])
XO_CHECK_FUNC([jpeg_start_decompress],[JPEG_START_DECOMPRESS])
XO_CHECK_FUNC([jpeg_std_error],[JPEG_STD_ERROR])
XO_CHECK_FUNC([jpeg_stdio_src],[JPEG_STDIO_SRC])
AC_MSG_NOTICE([])

# --------------------------------------------------------------------
# PRODUCTION
# --------------------------------------------------------------------
AC_CONFIG_FILES([Makefile src/Makefile man/Makefile])
AC_OUTPUT
AC_MSG_NOTICE([])
