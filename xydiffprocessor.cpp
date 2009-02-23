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

/* $Id: xydiffprocessor.cpp,v 1.16.2.1.2.1.2.1 2009/01/23 11:46:50 fdintino Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {
	#include "php.h"
	#include "php_ini.h"
	#include "ext/standard/info.h"

}
#include "include/php_xydiff.hpp"

#define XYDIFF_CLASS_NAME "XyDiff"

#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysctl.h>

#include "xercesc/framework/MemBufInputSource.hpp"
#include "xercesc/dom/DOMImplementation.hpp"
#include "xercesc/dom/DOMImplementationLS.hpp"
#include "xercesc/dom/DOMImplementationRegistry.hpp"
#include "xercesc/dom/DOMDocument.hpp"
#include "xercesc/dom/DOMLSParser.hpp"
#include "xercesc/dom/DOMLSOutput.hpp"
#include "xercesc/dom/DOMLSSerializer.hpp"
#include "xercesc/framework/Wrapper4InputSource.hpp"
#include "xercesc/framework/MemBufFormatTarget.hpp"
#include "xercesc/sax/ErrorHandler.hpp"
#include "xercesc/sax/SAXException.hpp"
#include "xercesc/sax/SAXParseException.hpp"
#include "xercesc/dom/DOMLocator.hpp"

#include "include/xydiffprocessor.h"
#include "include/xiddomdocument.h"

XERCES_CPP_NAMESPACE_USE

static zend_class_entry *xydiff_class_entry;

static zend_function_entry xydiff_methods[] = {
	ZEND_ME(xydiff, __construct, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xydiff, loadXML, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xydiff, diffXML, NULL, ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};
static zend_object_handlers xydiff_object_handlers;

void register_xydiff()
{
	memcpy(&xydiff_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, XYDIFF_CLASS_NAME, xydiff_methods);
	ce.create_object = xydiff_object_create;
	xydiff_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
}

static void xydiff_object_dtor(void *object TSRMLS_DC)
{
	xydiff_object *intern = (xydiff_object *)object;
	
	zend_object_std_dtor(&intern->std TSRMLS_CC);
	if (intern->xiddoc1) {
		intern->xiddoc1->release();
	}
	if (intern->xiddoc2) {
		intern->xiddoc2->release();
	}

	efree(object);
}

zend_object_value xydiff_object_create(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object_value retval;
	xydiff_object *intern;
	zval *tmp;

	intern = (xydiff_object *) emalloc(sizeof(xydiff_object));
	memset(intern, 0, sizeof(xydiff_object));
	intern->ptr1 = NULL;
	intern->doc1 = NULL;
	intern->doc2 = NULL;
	intern->xiddoc1 = NULL;
	intern->xiddoc2 = NULL;
	
	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	zend_hash_copy(intern->std.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
	
	retval.handle = zend_objects_store_put(intern,
										   (zend_objects_store_dtor_t)zend_objects_destroy_object,
										   (zend_objects_free_object_storage_t) xydiff_object_dtor, NULL TSRMLS_CC);
	intern->handle = retval.handle;
	retval.handlers = &xydiff_object_handlers;
	return retval;
	
}

static void xydiff_object_clone(void *object, void **object_clone TSRMLS_DC)
{
	xydiff_object *intern = (xydiff_object *) object;
	xydiff_object **intern_clone = (xydiff_object **) object_clone;

	*intern_clone = (xydiff_object *) emalloc(sizeof(xydiff_object));

	(*intern_clone)->xiddoc1 = XID_DOMDocument::copy(intern->xiddoc1, 1);
	(*intern_clone)->xiddoc2 = XID_DOMDocument::copy(intern->xiddoc2, 1);
}


const char * get_libxml_dom_string(php_libxml_node_object *doc, xmlChar* &mem, int &size)
{
	dom_doc_propsptr doc_props;
	int format = 0;
	
	xmlDocPtr docp = (xmlDocPtr) doc->document->ptr;
	doc_props = dom_get_doc_props(doc);
	format = doc_props->formatoutput;
	xmlDocDumpFormatMemory(docp, &mem, &size, format);
	efree(doc_props);
}

dom_doc_propsptr dom_get_doc_props(php_libxml_node_object *node)
{
	dom_doc_propsptr doc_props;
	php_libxml_ref_obj *document = node->document;
	
	if (document && document->doc_props) {
		return document->doc_props;
	} else {
		doc_props = (libxml_doc_props*) emalloc(sizeof(libxml_doc_props));
		doc_props->formatoutput = 0;
		doc_props->validateonparse = 0;
		doc_props->resolveexternals = 0;
		doc_props->preservewhitespace = 1;
		doc_props->substituteentities = 0;
		doc_props->stricterror = 1;
		doc_props->recover = 0;
		doc_props->classmap = NULL;
		if (document) {
			document->doc_props = doc_props;
		}
		return doc_props;
	}
}



static xmlDocPtr xid_domdocument_to_libxml_domdocument(XID_DOMDocument *xiddoc)
{
	DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(XMLString::transcode("LS"));
	DOMLSSerializer* theSerializer = ((DOMImplementationLS*)impl)->createLSSerializer();
	DOMLSOutput *theOutput = ((DOMImplementationLS*)impl)->createLSOutput();
	
	XMLFormatTarget *myFormatTarget = new MemBufFormatTarget();
	theOutput->setByteStream(myFormatTarget);
	
	try {
		if (theSerializer->getDomConfig()->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true))
			theSerializer->getDomConfig()->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);
		if (theSerializer->getDomConfig()->canSetParameter(XMLUni::fgDOMXMLDeclaration, true)) 
		    theSerializer->getDomConfig()->setParameter(XMLUni::fgDOMXMLDeclaration, true);		
		theSerializer->write((DOMDocument*)xiddoc, theOutput);
	}
	catch (const XMLException& toCatch) {
		std::cout << "Exception message is: \n" << XMLString::transcode(toCatch.getMessage()) << std::endl;
	}
	catch (const DOMException& toCatch) {
		std::cout << "Exception message is: \n" << XMLString::transcode(toCatch.getMessage()) << std::endl;
	}
	catch (...) {
		std::cout << "Unexpected Exception" << std::endl;
	}
	
	char* theXMLString_Encoded = (char*) ((MemBufFormatTarget*)myFormatTarget)->getRawBuffer();
	int xmlLen = (int) ((MemBufFormatTarget*)myFormatTarget)->getLen();
	
	char *xmlString = (char *) emalloc(sizeof(char)*xmlLen+1);
	strncpy (xmlString, theXMLString_Encoded, xmlLen);
	xmlString[xmlLen] = '\0';
	
	theOutput->release();
	theSerializer->release();
	
	
	xmlDocPtr newdoc;
	xmlParserCtxtPtr ctxt = NULL;
	
	ctxt = xmlCreateDocParserCtxt((xmlChar *)xmlString);
	if (ctxt == NULL) {
		newdoc = NULL;
	}
	xmlParseDocument(ctxt);
	if (ctxt->wellFormed) {
		newdoc = ctxt->myDoc;
	}

	efree(xmlString);
	return newdoc;
}

XID_DOMDocument * libxml_domdocument_to_xid_domdocument(php_libxml_node_object *libxml_doc)
{
	xmlChar *mem = NULL;
	int size = 0;
	XID_DOMDocument *xiddoc;
	dom_doc_propsptr doc_props;
	int format = 0;
	xmlDocPtr docp;

	docp = (xmlDocPtr) libxml_doc->document->ptr;
	doc_props = dom_get_doc_props(libxml_doc);
	format = doc_props->formatoutput;
	xmlDocDumpFormatMemory(docp, &mem, &size, format);
		
	const char *xmlString = (const char *)mem;
	

	try {
		XMLPlatformUtils::Initialize();
	}
	catch(const XMLException& toCatch) {
		char *message = strcat("XMLException: Error during Xerces-c Initialization:\n Exception message: ", XyLatinStr(toCatch.getMessage()).localForm());
		zend_throw_exception(zend_exception_get_default(TSRMLS_C),
							 message,
							 0 TSRMLS_CC);
	}
	
	static const XMLCh gLS[] = { chLatin_L, chLatin_S, chNull };
	try {
		DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(gLS);
		DOMLSParser *theParser = ((DOMImplementationLS*)impl)->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
		DOMErrorHandler * handler = new xydeltaParseHandler();
		XMLCh *myWideString = XMLString::transcode(xmlString);
		MemBufInputSource *memIS = new MemBufInputSource((const XMLByte *)xmlString, strlen(xmlString), "test", false);
		Wrapper4InputSource *wrapper = new Wrapper4InputSource(memIS, false);		
		theParser->getDomConfig()->setParameter(XMLUni::fgDOMErrorHandler, handler);
		xiddoc = (XID_DOMDocument *) theParser->parse((DOMLSInput *) wrapper);
	} catch (const XMLException& e) {
		zend_throw_exception(zend_exception_get_default(TSRMLS_C),
							 strcat("XMLException: An error occurred during parsing: ",XyLatinStr(e.getMessage()).localForm()),
							 0 TSRMLS_CC);
	} catch (const DOMException& toCatch) {
		char* message = XMLString::transcode(toCatch.msg);
		zend_throw_exception(zend_exception_get_default(TSRMLS_C),
							 strcat("DOMException: An error occurred during parsing: ",message),
							 0 TSRMLS_CC);
		XMLString::release(&message);
	} catch (...) {
		zend_throw_exception(zend_exception_get_default(TSRMLS_C),
							 "Unhandled exception in XML parsing",
							 0 TSRMLS_CC);
	}
	
	if (size) {
		xmlFree(mem);
	}
	return xiddoc;
}

ZEND_METHOD(xydiff, __construct)
{
	zval *id;
	xydiff_object *intern;
	xmlDoc *doc1p = NULL;
	xmlDoc *doc2p = NULL;
	const xmlChar *version = NULL;	
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, xydiff_class_entry) == FAILURE) {
		RETURN_FALSE;
	}
	
	doc1p = xmlNewDoc(version);
	if (!doc1p) {
		// Throw exception
		RETURN_FALSE;
	}
	intern = (xydiff_object *)zend_object_store_get_object(id TSRMLS_CC);
	
	if (intern != NULL) {
		intern->xiddoc1 = NULL;
		intern->xiddoc2 = NULL;
	}	
}

ZEND_METHOD(xydiff, loadXML)
{
	zval *id, *docp = NULL;
	xydiff_object *intern;
	
	php_libxml_node_object *xml_object;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oo", &id, xydiff_class_entry, &docp) == FAILURE) {
		RETURN_FALSE;
	}
	
	intern = (xydiff_object *)zend_object_store_get_object(id TSRMLS_CC);
	if (intern != NULL) {	
		xml_object = (php_libxml_node_object *) zend_object_store_get_object(docp TSRMLS_CC);
		xiddomdocument_sync_with_libxml(xml_object);
		intern->xiddoc1 = get_xiddomdocument(xml_object);
	}
}

ZEND_METHOD(xydiff, diffXML)
{
	zval *id, *docp = NULL;
	xmlDocPtr doc = NULL;
	xydiff_object *intern;
	xmlNode *nodep = NULL;
	php_libxml_node_object *xml_object;

	zval *rv;
	int ret;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oo", &id, xydiff_class_entry, &docp) == FAILURE) {
		RETURN_FALSE;
	}

	intern = (xydiff_object *)zend_object_store_get_object(id TSRMLS_CC);
	if (intern != NULL) {

		xml_object = (php_libxml_node_object *) zend_object_store_get_object(docp TSRMLS_CC);		
		xiddomdocument_sync_with_libxml(xml_object);

		intern->xiddoc2 = get_xiddomdocument(xml_object);
		nodep = php_libxml_import_node(docp TSRMLS_CC);
		if (nodep) {
			if (nodep->doc == NULL) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Imported Node must have associated Document");
				RETURN_NULL();
			}
			if (nodep->type == XML_DOCUMENT_NODE || nodep->type == XML_HTML_DOCUMENT_NODE) {
				nodep = xmlDocGetRootElement((xmlDocPtr) nodep);
			}
			doc = nodep->doc;
		}
		if (doc == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Document");
			return;
		}

		DOMDocument *deltaDoc = XyDelta::XyDiff(intern->xiddoc1, "doc1", (XID_DOMDocument *) intern->xiddoc2, "doc2", NULL);

		intern->libxml_delta_doc = xid_domdocument_to_libxml_domdocument((XID_DOMDocument*)deltaDoc);
		
	}
	
	if (!intern->libxml_delta_doc)
		RETURN_FALSE;
	
	DOM_RET_OBJ(rv, (xmlNodePtr) intern->libxml_delta_doc, &ret, NULL);
}