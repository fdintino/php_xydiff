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

#include "include/XyDelta_DOMInterface.hpp"
#include "include/xydiffprocessor.h"
#include "include/xiddomdocument.h"


XERCES_CPP_NAMESPACE_USE

static zend_class_entry *xydiff_class_entry;

static zend_function_entry xydiff_methods[] = {
	ZEND_ME(xydiff, __construct, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xydiff, setStartDocument, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xydiff, setEndDocument, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xydiff, createDelta, NULL, ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};
static zend_object_handlers xydiff_object_handlers;

static zend_class_entry *xydiff_exception_ce;

void register_xydiff()
{
	memcpy(&xydiff_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, XYDIFF_CLASS_NAME, xydiff_methods);
	ce.create_object = xydiff_object_create;
	xydiff_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
	zend_class_entry **xydiff_exception_ce_ptr;
	if (zend_hash_find(CG(class_table), "xydiffexception", sizeof("xydiffexception"), (void **) &xydiff_exception_ce_ptr) == FAILURE) {
		xydiff_exception_ce = zend_exception_get_default(TSRMLS_C);
	} else {
		xydiff_exception_ce = *xydiff_exception_ce_ptr;
	}
}

static void xydiff_object_dtor(void *object TSRMLS_DC)
{
	xydiff_object *intern = (xydiff_object *)object;
	zend_object_std_dtor(&intern->std TSRMLS_CC);
	efree(object);
}

zend_object_value xydiff_object_create(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object_value retval;
	xydiff_object *intern;
	zval *tmp;

	intern = (xydiff_object *) emalloc(sizeof(xydiff_object));
	memset(intern, 0, sizeof(xydiff_object));
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


ZEND_METHOD(xydiff, __construct)
{
	zval *id;
	xydiff_object *intern;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, xydiff_class_entry) == FAILURE) {
		RETURN_FALSE;
	}
	
	intern = (xydiff_object *)zend_object_store_get_object(id TSRMLS_CC);
	
	if (intern != NULL) {
		intern->xiddoc1 = NULL;
		intern->xiddoc2 = NULL;
	}	
}

ZEND_METHOD(xydiff, setStartDocument)
{
	zval *id, *docp = NULL;
	xydiff_object *intern;
	xmlNode *nodep = NULL;
	xmlDocPtr doc = NULL;
	php_libxml_node_object *xml_object;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oo", &id, xydiff_class_entry, &docp) == FAILURE) {
		RETURN_FALSE;
	}
	
	intern = (xydiff_object *)zend_object_store_get_object(id TSRMLS_CC);
	if (intern != NULL) {	
		xml_object = (php_libxml_node_object *) zend_object_store_get_object(docp TSRMLS_CC);
		
		// Do some sanity checks on the DOMDocument that was passed
		nodep = php_libxml_import_node(docp TSRMLS_CC);
		if (nodep) {
			if (nodep->doc == NULL) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Imported Node must have associated Document");
				RETURN_NULL();
			}
			if (nodep->type == XML_DOCUMENT_NODE || nodep->type == XML_HTML_DOCUMENT_NODE) {
				nodep = xmlDocGetRootElement((xmlDocPtr) nodep);
			}
			if (nodep == NULL) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Imported document has empty DOM tree");
				RETURN_NULL();
			}
			doc = nodep->doc;
		}
		if (doc == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Document");
			return;
		}
		
		
		xiddomdocument_sync_with_libxml(xml_object);
		intern->xiddoc1 = get_xiddomdocument(xml_object);
	}
}

ZEND_METHOD(xydiff, setEndDocument)
{
	zval *id, *docp = NULL;
	xmlDocPtr doc = NULL;
	xydiff_object *intern;
	xmlNode *nodep = NULL;
	php_libxml_node_object *xml_object;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oo", &id, xydiff_class_entry, &docp) == FAILURE) {
		RETURN_FALSE;
	}

	intern = (xydiff_object *)zend_object_store_get_object(id TSRMLS_CC);
	if (intern != NULL) {

		xml_object = (php_libxml_node_object *) zend_object_store_get_object(docp TSRMLS_CC);		

		// Do some sanity checks on the DOMDocument that was passed
		nodep = php_libxml_import_node(docp TSRMLS_CC);

		if (nodep) {
			if (nodep->doc == NULL) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Imported Node must have associated Document");
				RETURN_NULL();
			}
			if (nodep->type == XML_DOCUMENT_NODE || nodep->type == XML_HTML_DOCUMENT_NODE) {
				nodep = xmlDocGetRootElement((xmlDocPtr) nodep);
			}
			if (nodep == NULL) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Imported document has empty DOM tree");
				RETURN_NULL();
			}
			doc = nodep->doc;
		}
		if (doc == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Document");
			return;
		}
		xiddomdocument_sync_with_libxml(xml_object);
		intern->xiddoc2 = get_xiddomdocument(xml_object);
	}
}

ZEND_METHOD(xydiff, createDelta)
{
	zval *id, *rv;
	xydiff_object *intern;
	int ret;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, xydiff_class_entry) == FAILURE) {
		RETURN_FALSE;
	}
	
	intern = (xydiff_object *)zend_object_store_get_object(id TSRMLS_CC);
	if (intern != NULL) {
		if (intern->xiddoc1 == NULL) {
			zend_throw_exception(xydiff_exception_ce,
								 "No start document has been specified",
								 0 TSRMLS_CC);
			RETURN_FALSE;
		}
		if (intern->xiddoc2 == NULL) {
			zend_throw_exception(xydiff_exception_ce,
								 "No end document has been specified",
								 0 TSRMLS_CC);
			RETURN_FALSE;
		}

		XyDOMDelta* domDeltaCreate = new XyDOMDelta(intern->xiddoc1, intern->xiddoc2);
		XID_DOMDocument *deltaDoc = domDeltaCreate->createDelta();
		intern->libxml_delta_doc = xid_domdocument_to_libxml_domdocument(deltaDoc);
		deltaDoc->release();
		delete deltaDoc;
		delete domDeltaCreate;
		if (!intern->libxml_delta_doc)
			RETURN_FALSE;

		DOM_RET_OBJ(rv, (xmlNodePtr) intern->libxml_delta_doc, &ret, NULL);		
	}
}
