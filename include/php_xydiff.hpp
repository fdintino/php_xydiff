/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2008 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id: header,v 1.16.2.1.2.1.2.1 2008/02/07 19:39:50 iliaa Exp $ */

#ifndef PHP_XYDIFF_H
#define PHP_XYDIFF_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
}

#if !defined(_WIN32) && !defined(_WIN64)

#include <stdbool.h>
#include <unistd.h>
#include <sys/sysctl.h>

#endif

extern zend_module_entry xydiff_module_entry;
#define phpext_xydiff_ptr &xydiff_module_entry

#ifdef PHP_WIN32
#	define PHP_XYDIFF_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_XYDIFF_API __attribute__ ((visibility("default")))
#else
#	define PHP_XYDIFF_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif




#include "Tools.hpp"
#include "include/XID_map.hpp"

#include "include/XID_DOMDocument.hpp"
#include "include/XyDelta_DOMInterface.hpp"
#include "include/XyLatinStr.hpp"
#include "include/XID_DOMDocument.hpp"
#include "DeltaException.hpp"
#include "zend_exceptions.h"

#include <cstring>
// For libxml
extern "C" {
	#include "ext/libxml/php_libxml.h"
	#include "ext/dom/xml_common.h"
}

#include "xercesc/util/PlatformUtils.hpp"
#include "xercesc/dom/DOMErrorHandler.hpp"
#include "xercesc/dom/DOMException.hpp"
#include "xercesc/dom/DOMDocument.hpp"

#include "debug.h"

#define DebugBreak() if(AmIBeingDebugged()) {__asm__("int $3\n" : : );}

#define XYDIFF_CLASS_NAME "XyDiff"


static bool AmIBeingDebugged(void);



PHP_MINIT_FUNCTION(xydiff);
PHP_MSHUTDOWN_FUNCTION(xydiff);
PHP_RINIT_FUNCTION(xydiff);
PHP_RSHUTDOWN_FUNCTION(xydiff);
PHP_MINFO_FUNCTION(xydiff);




static xmlDocPtr string_to_dom_document(const char *source);
char * xiddomdocument_to_string(xercesc::DOMDocument *doc);


/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(xydiff)
	long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(xydiff)
*/

/* In every utility function you add that needs to use variables 
   in php_xydiff_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as XYDIFF_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#define XYDIFF_GET_OBJ1(__ptr, __id, __prtype, __intern) { \
__intern = (xydiff_object *)zend_object_store_get_object(__id TSRMLS_CC); \
if (__intern->ptr1 == NULL || !(__ptr = (__prtype)((php_libxml_node_ptr *)__intern->ptr1)->node)) { \
php_error_docref(NULL TSRMLS_CC, E_WARNING, "Couldn't fetch %s", __intern->std.ce->name);\
RETURN_NULL();\
} \
}

#ifdef ZTS
#define XYDIFF_G(v) TSRMG(xydiff_globals_id, zend_xydiff_globals *, v)
#else
#define XYDIFF_G(v) (xydiff_globals.v)
#endif

#endif	/* PHP_XYDIFF_H */
