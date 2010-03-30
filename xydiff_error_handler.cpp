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
	char *locatorUri = XMLString::transcode(locator->getURI());
	char *exceptionMsg = XMLString::transcode(domError.getMessage());
	sprintf(exception, "Error at (file %s, line %d, char %d): %s\n",
							  locatorUri,
							  locator->getLineNumber(),
							  locator->getColumnNumber(),
							  exceptionMsg);
	zend_throw_exception(xydiff_exception_ce, exception, 0 TSRMLS_CC);
	XMLString::release(&locatorUri);
	XMLString::release(&exceptionMsg);
	delete [] exception;
	throw VersionManagerException("xydiffPHPParseHandler", "error", "-");
}

void xydiffPHPParseHandler::error(const SAXParseException& e) {
	TSRMLS_FETCH();
	char *exception;
	char *exceptionMsg = XMLString::transcode(e.getMessage());
	sprintf(exception, "Error: %s\n", exceptionMsg);
	XMLString::release(&exceptionMsg);
	zend_throw_exception(xydiff_exception_ce, exception, 0 TSRMLS_CC);
	delete [] exception;

	throw VersionManagerException("xydiffPHPParseHandler", "error", "-");
}
void xydiffPHPParseHandler::fatalError(const SAXParseException& e) {
	TSRMLS_FETCH();
	char *exception;
	char *exceptionMsg = XMLString::transcode(e.getMessage());
	sprintf(exception, "Fatal Error: %s\n", exceptionMsg);
	XMLString::release(&exceptionMsg);
	zend_throw_exception(xydiff_exception_ce, exception, 0 TSRMLS_CC);
	delete [] exception;
	throw VersionManagerException("xydiffPHPParseHandler", "fatal error", "-");
}
void xydiffPHPParseHandler::warning(const SAXParseException& e) {
	char *exceptionMsg = XMLString::transcode(e.getMessage());
	std::cerr << "\n(GF) Warning: " << exceptionMsg << std::endl;
	XMLString::release(&exceptionMsg);
}

