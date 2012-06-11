/*
 *  xydiff_error_handler.h
 *  xydiff
 */


#ifndef XYDIFF_ERROR_HANDLER_H
#define XYDIFF_ERROR_HANDLER_H

#include "xydiff/XID_map.hpp"

#include "xercesc/dom/DOMDocument.hpp"
#include "xercesc/dom/DOMConfiguration.hpp"
#include "xercesc/dom/DOMError.hpp"
#include "xercesc/dom/DOMErrorHandler.hpp"
#include "xercesc/dom/DOMLSParser.hpp"
#include "xercesc/dom/DOMLocator.hpp"
#include <iostream>

#include "xercesc/sax/ErrorHandler.hpp"
#include "xercesc/sax/SAXException.hpp"
#include "xercesc/sax/SAXParseException.hpp"

#include "xydiff/VersionManagerException.hpp"
void register_xydiff_exception(TSRMLS_D);

class xydiffPHPParseHandler : public xercesc::DOMErrorHandler {
public:
	void warning(const xercesc::SAXParseException& e);
	void error(const xercesc::SAXParseException& e);
	void fatalError(const xercesc::SAXParseException& e);
	void resetErrors() {};
	bool handleError(const xercesc::DOMError& domError);
} ;

#endif