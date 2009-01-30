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
	#include "ext/libxml/php_libxml.h"
}
#include "include/php_xydiff.hpp"

#define XYDIFF_CLASS_NAME "XyDiff"

#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysctl.h>
//#include <stdio.h>
//#include <string.h>


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
	int retcount;
	
	zend_object_std_dtor(&intern->std TSRMLS_CC);
	if (intern->xiddoc1) {
		intern->xiddoc1->release();
	}
	if (intern->xiddoc2) {
		intern->xiddoc2->release();
	}
/*
	if (intern->doc1p != NULL && ((php_libxml_node_ptr *)intern->doc1p)->node != NULL) {
		if (((xmlNodePtr) ((php_libxml_node_ptr *)intern->doc1p)->node)->type != XML_DOCUMENT_NODE && ((xmlNodePtr) ((php_libxml_node_ptr *)intern->doc1p)->node)->type != XML_HTML_DOCUMENT_NODE) {
			php_libxml_node_decrement_resource((php_libxml_node_object *) intern TSRMLS_CC);
		} else {
			php_libxml_decrement_node_ptr((php_libxml_node_object *) intern TSRMLS_CC);
			retcount = php_libxml_decrement_doc_ref((php_libxml_node_object *)intern TSRMLS_CC);
		}
		intern->doc1p = NULL;
	}
	if (intern->doc2p != NULL && ((php_libxml_node_ptr *)intern->doc2p)->node != NULL) {
		if (((xmlNodePtr) ((php_libxml_node_ptr *)intern->doc2p)->node)->type != XML_DOCUMENT_NODE && ((xmlNodePtr) ((php_libxml_node_ptr *)intern->doc2p)->node)->type != XML_HTML_DOCUMENT_NODE) {
			php_libxml_node_decrement_resource((php_libxml_node_object *) intern TSRMLS_CC);
		} else {
			php_libxml_decrement_node_ptr((php_libxml_node_object *) intern TSRMLS_CC);
			retcount = php_libxml_decrement_doc_ref((php_libxml_node_object *)intern TSRMLS_CC);
		}
		intern->doc2p = NULL;
	}
*/
	if (intern->doc1) {
		php_libxml_decrement_doc_ref((php_libxml_node_object *) intern->doc1 TSRMLS_CC);
		efree(intern->doc1);
	}
	if (intern->doc2) {
		php_libxml_decrement_doc_ref((php_libxml_node_object *) intern->doc2 TSRMLS_CC);
		efree(intern->doc2);
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
	(*intern_clone)->doc1 = intern->doc1;
	(*intern_clone)->doc2 = intern->doc2;
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
}

// @todo Clean up potential memory leaks in this function
DOMDocument * string_to_xid_domdocument(xydiff_object *intern, const char *string)
{
	DOMDocument *theDocument;
	try {
		XMLPlatformUtils::Initialize();
	}
	catch(const XMLException& toCatch) {
		std::cerr << "Error during Xerces-c Initialization.\n"
		<< "  Exception message:" << XyLatinStr(toCatch.getMessage()).localForm() << std::endl;
		exit(-1);
	}

	static const XMLCh gLS[] = { chLatin_L, chLatin_S, chNull };
	DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(gLS);
	DOMLSParser *theParser = ((DOMImplementationLS*)impl)->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
	DOMErrorHandler * handler = new xydeltaParseHandler();
	XMLCh *myWideString = XMLString::transcode(string);
	MemBufInputSource *memIS = new MemBufInputSource((const XMLByte *)string, strlen(string), "test", false);
	Wrapper4InputSource *wrapper = new Wrapper4InputSource(memIS, false);
	try {
		theParser->getDomConfig()->setParameter(XMLUni::fgDOMErrorHandler, handler);
		theDocument = theParser->parse((DOMLSInput *) wrapper);
	} catch (const XMLException& e) {
		std::cerr << "XMLException: An error occured during parsing\n   Message: "
		<< XyLatinStr(e.getMessage()).localForm() << std::endl;
		throw VersionManagerException("XML Exception", "parseDOM_Document", "See previous exception messages for more details");
	} catch (const DOMException& toCatch) {
		char* message = XMLString::transcode(toCatch.msg);
		std::cout << "Exception message is: " << std::endl
		<< message << "\n";
		XMLString::release(&message);
	} catch (...) {
		std::cout << "Unexpected exception" << std::endl;
	}
	return theDocument;
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

ZEND_METHOD(xydiff, loadXML)
{
	zval *id, *doc1p = NULL;
	xmlDocPtr doc1 = NULL;
	xydiff_object *intern;
	xmlNode *node1p = NULL;
	
	php_libxml_node_object *xml_object1;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oo", &id, xydiff_class_entry, &doc1p) == FAILURE) {
		RETURN_FALSE;
	}
	
	intern = (xydiff_object *)zend_object_store_get_object(id TSRMLS_CC);
	if (intern != NULL) {
		xml_object1 = (php_libxml_node_object *)zend_object_store_get_object(doc1p TSRMLS_CC);
		node1p = php_libxml_import_node(doc1p TSRMLS_CC);
		if (node1p) {
			doc1 = node1p->doc;
		}
		if (doc1 == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Document");
			return;
		}
		
		intern->doc1 = (php_libxml_node_object*) emalloc(sizeof(php_libxml_node_object));
		memset(intern->doc1, 0, sizeof(php_libxml_node_object));
		intern->doc1->document = xml_object1->document;
		
		php_libxml_increment_doc_ref(intern->doc1, doc1);	
	}
}

ZEND_METHOD(xydiff, diffXML)
{
	zval *id, *doc2p = NULL;
	xmlDocPtr doc2 = NULL;
	xydiff_object *intern;
	xmlNode *node2p = NULL;

	php_libxml_node_object *xml_object2;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oo", &id, xydiff_class_entry, &doc2p) == FAILURE) {
		RETURN_FALSE;
	}
	
	intern = (xydiff_object *)zend_object_store_get_object(id TSRMLS_CC);
	if (intern != NULL) {
		xml_object2 = (php_libxml_node_object *)zend_object_store_get_object(doc2p TSRMLS_CC);
		node2p = php_libxml_import_node(doc2p TSRMLS_CC);
		if (node2p) {
			doc2 = node2p->doc;
		}
		if (doc2 == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Document");
			return;
		}
		
		intern->doc2 = (php_libxml_node_object*) emalloc(sizeof(php_libxml_node_object));
		memset(intern->doc2, 0, sizeof(php_libxml_node_object));
		intern->doc2->document = xml_object2->document;
		php_libxml_increment_doc_ref(intern->doc2, doc2);
	}
	
	xmlChar *mem1, *mem2 = NULL;
	int size1, size2 = 0;
	const char *text1 = get_libxml_dom_string(intern->doc1, mem1, size1);
	const char *text2 = get_libxml_dom_string(intern->doc2, mem2, size2);
	
	intern->xiddoc1 = (XID_DOMDocument *) string_to_xid_domdocument(intern, (const char *)mem1 );
	intern->xiddoc2 = (XID_DOMDocument *) string_to_xid_domdocument(intern, (const char *)mem2 );
	DOMDocument *deltaDoc = XyDelta::XyDiff(intern->xiddoc1, "doc1", intern->xiddoc2, "doc2", NULL);

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
		theSerializer->write((DOMDocument*)deltaDoc, theOutput);
	}
	catch (const XMLException& toCatch) {
		char* message = XMLString::transcode(toCatch.getMessage());
		std::cout << "Exception message is: \n" << message << std::endl;
		XMLString::release(&message);
	}
	catch (const DOMException& toCatch) {
		char* message = XMLString::transcode(toCatch.msg);
		std::cout << "Exception message is: \n" << message << std::endl;
		XMLString::release(&message);
	}
	catch (...) {
		std::cout << "Unexpected Exception" << std::endl;
	}

	char* theXMLString_Encoded = (char*) ((MemBufFormatTarget*)myFormatTarget)->getRawBuffer();
	int xmlLen = (int) ((MemBufFormatTarget*)myFormatTarget)->getLen();
	//char xmlString[xmlLen+1];
	char *xmlString = (char *) emalloc(xmlLen+1);
	strncpy (xmlString, theXMLString_Encoded, xmlLen);
	xmlString[xmlLen] = '\0';
	
	// string_to_dom_document(xmlString);
	RETVAL_STRINGL(xmlString, xmlLen, 1);
	efree(xmlString);
	// Free memory
	if (size1) {
		xmlFree(mem1);
	}
	if (size2) {
		xmlFree(mem2);
	}

	theOutput->release();
	theSerializer->release();
}

xmlDocPtr string_to_dom_document(char *source)
{
	xmlDoc *xDoc = NULL;
	//xmlDoc *xDoc = dom_document_parser(NULL, 0, source, 0);
	return xDoc;
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
		intern->doc1 = NULL;
		intern->doc2 = NULL;
		//if (php_libxml_increment_doc_ref((php_libxml_node_object *)intern->doc1p, doc1p TSRMLS_CC) == -1) {
		//	RETURN_FALSE;
		//}
	//	php_libxml_increment_node_ptr((php_libxml_node_object *)intern->doc1p, (xmlNodePtr)doc1p, (void *)intern TSRMLS_CC);
	}
	
}

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_xydiff_compiled(string arg)
 Return a string to confirm that the module is compiled in */


/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_xydiff_compiled(string arg)
 Return a string to confirm that the module is compiled in */

