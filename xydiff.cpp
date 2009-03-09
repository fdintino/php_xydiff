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


#include <assert.h>
#include <sys/types.h>

#include "include/php_xydiff.hpp"
#include "include/xydiffprocessor.h"
#include "include/xiddomdocument.h"
#include "include/xydiff_error_handler.h"
#include "include/xydelta.h"

XERCES_CPP_NAMESPACE_USE

/* If you declare any globals in php_xydiff.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(xydiff)
*/

zend_function_entry xydiff_functions[] = {
	{NULL, NULL, NULL}	/* Must be the last line in xydiff_functions[] */
};


#if ZEND_MODULE_API_NO >= 20050617 
static zend_module_dep xydiff_deps[] = {
	ZEND_MOD_REQUIRED("dom")
	{NULL, NULL, NULL}
};
#endif



zend_module_entry xydiff_module_entry = {
#if ZEND_MODULE_API_NO >= 20050617 
	STANDARD_MODULE_HEADER_EX, NULL,
	xydiff_deps,
#elif ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"xydiff",
	xydiff_functions,
	PHP_MINIT(xydiff),
	PHP_MSHUTDOWN(xydiff),
	PHP_RINIT(xydiff),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(xydiff),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(xydiff),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_XYDIFF
BEGIN_EXTERN_C()
ZEND_GET_MODULE(xydiff)
END_EXTERN_C()
#endif


/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("xydiff.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_xydiff_globals, xydiff_globals)
    STD_PHP_INI_ENTRY("xydiff.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_xydiff_globals, xydiff_globals)
PHP_INI_END()
*/


/* Uncomment this function if you have INI entries
static void php_xydiff_init_globals(zend_xydiff_globals *xydiff_globals)
{
	xydiff_globals->global_value = 0;
	xydiff_globals->global_string = NULL;
}
*/

static HashTable classes;


static zend_class_entry *xiddomdocument_class_entry;

PHP_MINIT_FUNCTION(xydiff)
{
	
	try {
		XMLPlatformUtils::Initialize();
	}
	catch(const XMLException& toCatch) {
		char *message = strcat("XMLException: Error during Xerces-c Initialization:\n Exception message: ", XyLatinStr(toCatch.getMessage()).localForm());
		zend_throw_exception(zend_exception_get_default(TSRMLS_C),
							 message,
							 0 TSRMLS_CC);
	}
	// The XyDiffException class has to be registered first for the other two classes to access it
	register_xydiff_exception(TSRMLS_C);
	register_xydiff(TSRMLS_C);
	register_xiddomdocument(TSRMLS_C);
	register_xydelta(TSRMLS_C);
	
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/

	
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(xydiff)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}


/* Remove if there's nothing to do at request start */
PHP_RINIT_FUNCTION(xydiff)
{
	return SUCCESS;
}

/* Remove if there's nothing to do at request end */
PHP_RSHUTDOWN_FUNCTION(xydiff)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(xydiff)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "xydiff support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}