/*
 *  xydiff_error_handler.cpp
 *  xydiff
 */

#include "include/php_xydiff.hpp"

#include <cstring>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
}

#include "include/xydiff_error_handler.h"

XERCES_CPP_NAMESPACE_USE

static zend_function_entry xydiff_exception_functions[] = {
	{NULL, NULL, NULL}
};
static zend_class_entry *xydiff_exception_ce;
static zend_class_entry *xy_xml_exception_ce;
static zend_class_entry *xy_dom_exception_ce;

void register_xydiff_exception(TSRMLS_D)
{
	zend_class_entry ce_xydiff;
	INIT_CLASS_ENTRY(ce_xydiff, "XyDiffException", xydiff_exception_functions);
	xydiff_exception_ce = zend_register_internal_class_ex(&ce_xydiff, zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
	zend_class_entry ce_xyxml;
	INIT_CLASS_ENTRY(ce_xyxml, "XyXMLException", xydiff_exception_functions);
	xy_xml_exception_ce = zend_register_internal_class_ex(&ce_xyxml, zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
	zend_class_entry ce_xydom;
	INIT_CLASS_ENTRY(ce_xydom, "XyDOMException", xydiff_exception_functions);
	xy_dom_exception_ce = zend_register_internal_class_ex(&ce_xydom, zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
}

bool xydiffPHPParseHandler::handleError(const DOMError& domError)
{
	TSRMLS_FETCH();
	DOMLocator* locator = domError.getLocation();
	char *exception;
	sprintf(exception, "Error at (file %s, line %d, char %d): %s\n",
							  XMLString::transcode(locator->getURI()),
							  locator->getLineNumber(),
							  locator->getColumnNumber(),
							  XMLString::transcode(domError.getMessage()));
	zend_throw_exception(xydiff_exception_ce,
						exception,
						 0 TSRMLS_CC);
	delete [] exception;
	throw VersionManagerException("xydiffPHPParseHandler", "error", "-");
}

void xydiffPHPParseHandler::error(const SAXParseException& e) {
	TSRMLS_FETCH();
	char *exception;
	sprintf(exception, "Error at (file %s, line %d, char %d): %s\n",
			XMLString::transcode(e.getSystemId()),
			e.getLineNumber(),
			e.getColumnNumber(),
			XMLString::transcode(e.getMessage()));
	zend_throw_exception(xydiff_exception_ce,
						 exception,
						 0 TSRMLS_CC);
	delete [] exception;
	throw VersionManagerException("xydiffPHPParseHandler", "error", "-");
}
void xydiffPHPParseHandler::fatalError(const SAXParseException& e) {
	TSRMLS_FETCH();
	char *exception;
	sprintf(exception, "Fatal Error at (file %s, line %d, char %d): %s\n",
			XMLString::transcode(e.getSystemId()),
			e.getLineNumber(),
			e.getColumnNumber(),
			XMLString::transcode(e.getMessage()));
	zend_throw_exception(xydiff_exception_ce,
						 exception,
						 0 TSRMLS_CC);
	delete [] exception;
	throw VersionManagerException("xydiffPHPParseHandler", "fatal error", "-");
}
void xydiffPHPParseHandler::warning(const SAXParseException& e) {
	std::cerr << "\n(GF) Warning at (file " << XMLString::transcode(e.getSystemId())
	<< ", line " << e.getLineNumber()
	<< ", char " << e.getColumnNumber()
	<< "): " << XMLString::transcode(e.getMessage()) << std::endl;
}

