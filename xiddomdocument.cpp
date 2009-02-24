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

#include <zend_interfaces.h>

XERCES_CPP_NAMESPACE_USE

static zend_class_entry *xiddomdocument_class_entry;

static zend_function_entry xiddomdocument_methods[] = {
	ZEND_ME(xiddomdocument, __construct, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xiddomdocument, __destruct, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xiddomdocument, getXidMap, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xiddomdocument, generateXidTaggedDocument, NULL, ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};

static zend_object_handlers xiddomdocument_object_handlers;

zend_object_value xiddomdocument_object_create(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object_value retval;
	dom_object *intern;
	zval *tmp;
	
	intern = (dom_object *) emalloc(sizeof(dom_object));
	memset(intern, 0, sizeof(dom_object));
	intern->ptr = NULL;
	intern->prop_handler = NULL;
	intern->document = NULL;
	
	zend_class_entry *base_class;
	base_class = class_type;
	while(base_class->type != ZEND_INTERNAL_CLASS && base_class->parent != NULL) {
		base_class = base_class->parent;
	}
	zend_class_entry **pce;
	if (zend_lookup_class("DOMDocument", strlen("DOMDocument"), &pce TSRMLS_CC) == FAILURE) {
	}
	
	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	zend_hash_copy(intern->std.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
	
	retval.handle = zend_objects_store_put(intern,
										   (zend_objects_store_dtor_t)zend_objects_destroy_object,
										   (zend_objects_free_object_storage_t) xiddomdocument_object_dtor, NULL TSRMLS_CC);
	intern->handle = retval.handle;
	retval.handlers = &xiddomdocument_object_handlers;
	return retval;
}

static void xiddomdocument_object_dtor(void *object TSRMLS_DC)
{
	dom_object *intern;
	php_libxml_node_object *xml_object;
	intern = (dom_object *) object;
	xml_object = (php_libxml_node_object *) object;

	XID_DOMDocument *xiddoc = get_xiddomdocument(xml_object);
	if (xiddoc) {
		xiddoc->release();
		xiddoc = NULL;
	}
	int refcount = php_libxml_decrement_node_ptr((php_libxml_node_object *)intern TSRMLS_CC);
	php_libxml_decrement_doc_ref((php_libxml_node_object *)intern TSRMLS_CC);
	efree(object);
}


void register_xiddomdocument(TSRMLS_DC)
{
	memcpy(&xiddomdocument_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	zend_class_entry ce;
	zend_class_entry **pce;
	//	zend_class_entry *pce;
	if (zend_lookup_class("DOMDocument", strlen("DOMDocument"), &pce TSRMLS_CC) == FAILURE) {
		return;
	}
	INIT_CLASS_ENTRY(ce, "XIDDOMDocument", xiddomdocument_methods);
	ce.create_object = xiddomdocument_object_create;
	xiddomdocument_class_entry = zend_register_internal_class_ex(&ce TSRMLS_CC, pce[0], NULL TSRMLS_CC);
}

static void xiddomdocument_object_clone(void *object, void **object_clone TSRMLS_DC)
{
	dom_object *intern = (dom_object *) object;
	dom_object **intern_clone = (dom_object **) object_clone;
	
	*intern_clone = (dom_object *) emalloc(sizeof(dom_object));
	//(*intern_clone)->ptr = XID_DOMDocument::copy( (XID_DOMDocument *)intern->ptr , 1);
}

void xiddomdocument_sync_with_libxml(php_libxml_node_object *libxml_object)
{
	XID_DOMDocument *xiddoc = libxml_domdocument_to_xid_domdocument(libxml_object);
	uintptr_t xiddocptr = (uintptr_t) xiddoc;
	zend_hash_update(libxml_object->properties, "xiddoc", sizeof("xiddoc"), &xiddocptr, sizeof(uintptr_t), NULL);		
}

XID_DOMDocument * get_xiddomdocument(php_libxml_node_object *object)
{
	uintptr_t *xiddocptr;
	zend_hash_find(object->properties, "xiddoc", sizeof("xiddoc"), (void **) &xiddocptr );
	if (xiddocptr != NULL) {
		return (XID_DOMDocument *) xiddocptr[0];
	}
}

void propDestructor(void *pElement);
void propDestructor(void *pElement)
{
	pElement = NULL;
}

ZEND_METHOD(xiddomdocument, __destruct)
{
	zval *id;
	php_libxml_node_object *xml_object;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, xiddomdocument_class_entry) == FAILURE) {
		return;
	}
	xml_object = (php_libxml_node_object *) zend_object_store_get_object(id TSRMLS_CC);
	zend_hash_destroy(xml_object->properties);
	FREE_HASHTABLE(xml_object->properties);
}

ZEND_METHOD(xiddomdocument, getXidMap)
{
	zval *id;
	php_libxml_node_object *intern;
	XID_DOMDocument *xiddoc;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, xiddomdocument_class_entry) == FAILURE) {
		return;
	}

	intern = (php_libxml_node_object *) zend_object_store_get_object(id TSRMLS_CC);
	xiddoc = get_xiddomdocument(intern);
	const char *xidmap = xiddoc->getXidMap().String().c_str();
	RETVAL_STRINGL(xidmap, strlen(xidmap), true);

}

ZEND_METHOD(xiddomdocument, generateXidTaggedDocument)
{
	zval *id, *rv;
	php_libxml_node_object *intern;
	XID_DOMDocument *xiddoc;
	int ret;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, xiddomdocument_class_entry) == FAILURE) {
		return;
	}
	intern = (php_libxml_node_object *) zend_object_store_get_object(id TSRMLS_CC);
	xiddoc = get_xiddomdocument(intern);
	try {
		XID_DOMDocument* d = XID_DOMDocument::copy(xiddoc);
		
		DOMNode* root = (DOMNode *) d->getDocumentElement();
		if (root!=NULL) Restricted::XidTagSubtree(d, root);
		xmlDocPtr libxmldoc = xid_domdocument_to_libxml_domdocument(d);
		
		//d->release();
		//delete d;
		if (!libxmldoc)
			RETURN_FALSE;
		DOM_RET_OBJ(rv, (xmlNodePtr) libxmldoc, &ret, NULL);
		
	}
	catch( const VersionManagerException &e ) {
		zend_throw_exception(zend_exception_get_default(TSRMLS_C),
							 strcat("VersionManagerException: ", e.message.c_str() ),
							 0 TSRMLS_CC);
	}
	catch( const DOMException &e ) {
		zend_throw_exception(zend_exception_get_default(TSRMLS_C),
							 strcat("DOMException: ", XMLString::transcode(e.msg) ),
							 0 TSRMLS_CC);
	}

}

ZEND_METHOD(xiddomdocument, __construct)
{
	zval *id;
	dom_object *intern;
	zend_class_entry **pce;
	zend_class_entry *ce = xiddomdocument_class_entry;
	
	char *encoding, *version = NULL;
	int encoding_len = 0, version_len = 0, refcount;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O|ss", &id, xiddomdocument_class_entry, &version, &version_len, &encoding, &encoding_len) == FAILURE) {
		return;
	}

	intern = (dom_object *)zend_object_store_get_object(id TSRMLS_CC);

	// Call parent constructor

	if (zend_lookup_class("DOMDocument", strlen("DOMDocument"), &pce TSRMLS_CC) == FAILURE) {
		return;
	}

	zval *self = getThis();
	zend_function *ctor = ce->parent ? ce->parent->constructor : NULL;
	if (ctor) {
		zend_call_method_with_0_params(&self, ce, &ctor, ZEND_CONSTRUCTOR_FUNC_NAME, NULL);
	}
	
	php_libxml_node_object *xml_object = (php_libxml_node_object *) intern;
	ALLOC_HASHTABLE(xml_object->properties);
	if (zend_hash_init(xml_object->properties, 50, NULL, (dtor_func_t) propDestructor, 0) == FAILURE) {
		FREE_HASHTABLE(xml_object->properties);
		return;
	}
	
//	zval *encoding_str;
//	zval *version_str;
//	if (version_len == 0) {
//		version = "1.0";
//		MAKE_STD_ZVAL(version_str);
//		ZVAL_STRING(version_str, version, 1);		
//	}
//	if (encoding_len > 0) {
//		encoding = "iso-8859-1";
//		MAKE_STD_ZVAL(encoding_str);
//		ZVAL_STRING(encoding_str, encoding, 1);
//	}
//	if (encoding_len > 0) {
//		zend_call_method_with_2_params(&self, intern->pce_ptr, NULL, "load", NULL, version_str, encoding_str);
//	} else if (version_len == 0) {
//		
//		zend_call_method_with_1_params(&self, intern->pce_ptr, NULL, "__construct", NULL, version_str);
//	} else {
//		zend_call_method_with_0_params(&self, ce->parent, NULL, "__construct", NULL);
//	}
	
//	zval *libxml_id;
//	php_libxml_node_object *libxml_object;
//	
//	ALLOC_ZVAL(libxml_id);
//	object_init_ex(libxml_id, intern->pce_ptr);
//	libxml_object = (php_libxml_node_object *)zend_object_store_get_object(libxml_id TSRMLS_CC);
//	intern->doc = libxml_object;
//	FREE_ZVAL(libxml_id);
	
//	FREE_ZVAL(encoding_str);
//	FREE_ZVAL(version_str);
}



xmlDocPtr xid_domdocument_to_libxml_domdocument(XID_DOMDocument *xiddoc)
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
	DOMDocument *xiddoc;
	dom_doc_propsptr doc_props;
	int format = 0;
	xmlDocPtr docp;
	
	docp = (xmlDocPtr) libxml_doc->document->ptr;
	doc_props = dom_get_doc_props(libxml_doc);
	format = doc_props->formatoutput;
	xmlDocDumpFormatMemory(docp, &mem, &size, format);
	
	const char *xmlString = (const char *)mem;
	
	static const XMLCh gLS[] = { chLatin_L, chLatin_S, chNull };
	try {
		DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(gLS);
		DOMLSParser *theParser = ((DOMImplementationLS*)impl)->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
		DOMErrorHandler * handler = new xydeltaParseHandler();
		XMLCh *myWideString = XMLString::transcode(xmlString);
		MemBufInputSource *memIS = new MemBufInputSource((const XMLByte *)xmlString, strlen(xmlString), "test", false);
		Wrapper4InputSource *wrapper = new Wrapper4InputSource(memIS, false);		
		theParser->getDomConfig()->setParameter(XMLUni::fgDOMErrorHandler, handler);
		xiddoc = theParser->parse((DOMLSInput *) wrapper);
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
	XID_DOMDocument *xiddomdoc = new XID_DOMDocument(xiddoc);
	return xiddomdoc;
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