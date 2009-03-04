dnl $Id: Extending_Zend_Build.xml,v 1.8 2002/10/10 18:13:11 imajes Exp $
dnl config.m4 for extension xydiff


PHP_ARG_WITH(xydiff, for xydiff support,
[  --with-xydiff[=DIR]	Support for xydiff.])

if test -z "$PHP_XERCESC_PREFIX"; then
	PHP_ARG_WITH(xercesc-dir, xercesc install dir,
	[  --with-xercesc-dir[=DIR]   XyDiff: xercesc install prefix], no, no)
fi

if test -z "$PHP_LIBXML_PREFIX"; then
  PHP_ARG_WITH(libxml-dir, libxml2 install dir,
  [  --with-libxml-dir[=DIR]   XyDiff: libxml2 install prefix], no, no)
fi


AC_DEFUN([LIBXML_CHECK_HEADER],[
	SEARCH_FOR=$1
	LIBXML_SEARCH_PATH="/usr /usr/local /opt/local $PHP_LIBXML_PREFIX"
	AC_MSG_CHECKING([for $1])
	for i in $LIBXML_SEARCH_PATH ; do
		if test -z "$LIBXML_PREFIX"; then
			if test -f $i/include/libxml2/$SEARCH_FOR; then
				LIBXML_PREFIX=$i
				AC_MSG_RESULT(found in $i)
			fi
		fi
 	done
	if test -z "$LIBXML_PREFIX"; then
		AC_MSG_RESULT(no)
		AC_MSG_ERROR([libxml2 include directory could not be found])
	fi
])

AC_DEFUN([XYDIFF_CHECK_HEADER],[
	SEARCH_FOR=$1
	XYDIFF_SEARCH_PATH="/usr /usr/local /opt/local $PHP_XYDIFF"
	AC_MSG_CHECKING([for $1])
	for i in $XYDIFF_SEARCH_PATH ; do
		if test -z "$XYDIFF_LIB_DIR"; then
			if test -f $i/$SEARCH_FOR; then
				XYDIFF_LIB_DIR=$i
				AC_MSG_RESULT(found in $i)
			fi
		fi
 	done
	if test -z "$XYDIFF_LIB_DIR"; then
		AC_MSG_RESULT(no)
		AC_MSG_ERROR([XyDiff include directory could not be found])
	fi
])

AC_DEFUN([XERCESC_CHECK_HEADER],[
	SEARCH_FOR=$1
	XERCESC_SEARCH_PATH="/usr /usr/local /opt/local $PHP_XERCESC_PREFIX"
	AC_MSG_CHECKING([for $1])
	for i in $XERCESC_SEARCH_PATH ; do
		if test -z "$XERCESC_PREFIX"; then
			if test -f $i/include/xercesc/$SEARCH_FOR; then
				XERCESC_PREFIX=$i
				AC_MSG_RESULT(found in $i)
			fi
		fi
 	done
	if test -z "$XERCESC_PREFIX"; then
		AC_MSG_RESULT(no)
		AC_MSG_ERROR([Xerces-c++ include directory could not be found])
	fi
])

if test "$PHP_XYDIFF" != "no"; then
	PHP_REQUIRE_CXX


	XERCESC_CHECK_HEADER(dom/DOM.hpp)
	PHP_ADD_INCLUDE($XERCESC_PREFIX/include)
	PHP_ADD_LIBRARY_WITH_PATH(xerces-c, $XERCESC_PREFIX/lib, XYDIFF_SHARED_LIBADD)

	XYDIFF_CHECK_HEADER(include/XID_DOMDocument.hpp)

	# --with-xydiff -> add include path
	PHP_ADD_INCLUDE($XYDIFF_LIB_DIR)
	PHP_ADD_INCLUDE($XYDIFF_LIB_DIR/include)

	# --with-xydiff -> check for library
	if test ! -f "$XYDIFF_LIB_DIR/lib/libXyDelta.a"; then
		AC_MSG_ERROR([libXyDelta.dylib not found in $XYDIFF_LIB_DIR/lib])
	fi

	# --with-xydiff -> add library
	LIBNAME=xydiff
	PHP_SUBST(XYDIFF_SHARED_LIBADD)
	PHP_ADD_LIBRARY_WITH_PATH(XyDelta, $XYDIFF_LIB_DIR/lib, XYDIFF_SHARED_LIBADD)
	PHP_ADD_LIBRARY_WITH_PATH(stdc++, "", XYDIFF_SHARED_LIBADD)

	LIBXML_CHECK_HEADER(libxml/parser.h)
	PHP_ADD_INCLUDE($LIBXML_PREFIX/include/libxml2)
	PHP_ADD_LIBRARY_WITH_PATH(xml2, $LIBXML_PREFIX/lib, XYDIFF_SHARED_LIBADD)
	PHP_ADD_LIBRARY(z, XYDIFF_SHARED_LIBADD)
	PHP_ADD_LIBRARY(m, XYDIFF_SHARED_LIBADD)

	AC_DEFINE(HAVE_XYDIFF,1,[ ])
	PHP_ADD_EXTENSION_DEP(xydiff, dom)
	PHP_ADD_EXTENSION_DEP(xydiff, libxml)
	PHP_NEW_EXTENSION(xydiff, [xydiff.cpp \
	                           xydiffprocessor.cpp \
	                           xiddomdocument.cpp \
	                           xydiff_error_handler.cpp], $ext_shared,,,1)
fi