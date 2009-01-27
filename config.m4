dnl $Id: Extending_Zend_Build.xml,v 1.8 2002/10/10 18:13:11 imajes Exp $
dnl config.m4 for extension xydiff


PHP_ARG_WITH(xydiff, for xydiff support,
[	--with-xydiff=DIR	Support for xydiff.])

dnl PHP_ARG_ENABLE(xydiff, for xydiff support,
dnl [	--enable-xydiff	Support for xydiff.])
if test "$PHP_XYDIFF" != "no"; then
	PHP_REQUIRE_CXX
	
	# --with-my_module -> check with-path
	SEARCH_PATH="/usr/local /usr /opt/local"
	SEARCH_FOR="/include/DeltaApply.hpp"
	if test "$PHP_XYDIFF" = "yes" -a -r $PHP_XYDIFF/; then
	 # search default path list
		AC_MSG_CHECKING([for xydiff files in default path $PHP_XYDIFF])
		for i in $SEARCH_PATH ; do
			if test -f $i/$SEARCH_FOR; then
				XYDIFF_LIB_DIR=$i
				AC_MSG_RESULT(found in $i)
			fi
		done
	else
	XYDIFF_LIB_DIR=$PHP_XYDIFF
	fi

	# --with-xydiff -> add include path
	PHP_ADD_INCLUDE($XYDIFF_LIB_DIR/include)

	# --with-xydiff -> check for library
	if test ! -f "$XYDIFF_LIB_DIR/lib/libXyDelta.dylib"; then
		AC_MSG_ERROR([libXyDelta.dylib not found in $XYDIFF_LIB_DIR])
	fi

	# --with-xydiff -> add library
	LIBNAME=xydiff
	PHP_SUBST(XYDIFF_SHARED_LIBADD)
	PHP_ADD_LIBRARY_WITH_PATH(xerces-c, $XYDIFF_LIB_DIR/lib, XYDIFF_SHARED_LIBADD)
	PHP_ADD_LIBRARY_WITH_PATH(XyDelta, $XYDIFF_LIB_DIR/lib, XYDIFF_SHARED_LIBADD)
	PHP_ADD_LIBRARY_WITH_PATH(stdc++, "", XYDIFF_SHARED_LIBADD)

	AC_DEFINE(HAVE_XYDIFF,1,[ ])
	PHP_NEW_EXTENSION(xydiff, xydiff.cpp xydiffprocessor.cpp, $ext_shared,,,1)
	
	
fi