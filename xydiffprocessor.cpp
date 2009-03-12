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
#include <sys/types.h>

#include "xercesc/framework/MemBufInputSource.hpp"
#include "xercesc/dom/DOMImplementation.hpp"
#include "xercesc/dom/DOMImplementationLS.hpp"
#include "xercesc/dom/DOMImplementationRegistry.hpp"
#include "xercesc/dom/DOMDocument.hpp"
#include "xercesc/dom/DOMElement.hpp"
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

extern "C" {
#include "zend_objects_API.h"
}

XERCES_CPP_NAMESPACE_USE

static zend_class_entry *xydiff_class_entry;

static zend_function_entry xydiff_methods[] = {
	ZEND_ME(xydiff, __construct, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xydiff, createDelta, NULL, ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};
static zend_object_handlers xydiff_object_handlers;

static zend_class_entry *xydiff_exception_ce;

void register_xydiff(TSRMLS_D) {
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

static void xydiff_object_dtor(void *object TSRMLS_DC) {
	xydiff_object *intern = (xydiff_object *)object;
	zend_object_std_dtor(&intern->std TSRMLS_CC);
	efree(object);
}

zend_object_value xydiff_object_create(zend_class_entry *class_type TSRMLS_DC) {
	zend_object_value retval;
	xydiff_object *intern;
	zval *tmp;

	intern = (xydiff_object *) emalloc(sizeof(xydiff_object));
	memset(intern, 0, sizeof(xydiff_object));
	
	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	zend_hash_copy(intern->std.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
	
	retval.handle = zend_objects_store_put(intern,
										   (zend_objects_store_dtor_t)zend_objects_destroy_object,
										   (zend_objects_free_object_storage_t) xydiff_object_dtor, NULL TSRMLS_CC);
	intern->handle = retval.handle;
	retval.handlers = &xydiff_object_handlers;
	return retval;
	
}

static void xydiff_object_clone(void *object, void **object_clone TSRMLS_DC) {
	xydiff_object *intern = (xydiff_object *) object;
	xydiff_object **intern_clone = (xydiff_object **) object_clone;

	*intern_clone = (xydiff_object *) emalloc(sizeof(xydiff_object));
}

ZEND_METHOD(xydiff, __construct) {
	zval *id;
	xydiff_object *intern;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, xydiff_class_entry) == FAILURE) {
		RETURN_FALSE;
	}
	intern = (xydiff_object *)zend_object_store_get_object(id TSRMLS_CC);
}

ZEND_METHOD(xydiff, createDelta) {
	zval *id = NULL;
	xydiff_object *intern;
	zval *doc1, *doc2 = NULL;
	xmlNode *node1, *node2 = NULL;
	php_libxml_node_object *xml_object1, *xml_object2 = NULL;
	XID_DOMDocument *xiddoc1, *xiddoc2, *deltaDoc = NULL;
	xmlDocPtr libxml_delta_doc;
	char *xidmap = NULL;
	zval *rv = NULL;
	int ret;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ooo", &id, xydiff_class_entry, &doc1, &doc2) == FAILURE) {
		RETURN_FALSE;
	}

	intern = (xydiff_object *)zend_object_store_get_object(id TSRMLS_CC);
	if (intern != NULL) {

		node1 = php_libxml_import_node(doc1 TSRMLS_CC);
		node2 = php_libxml_import_node(doc2 TSRMLS_CC);

		// Do some sanity checks on the DOMDocuments that were passed
		char *error_buf;

		if (SUCCESS != xydiff_check_libxml_document(node1, &error_buf)) {
			char error_msg [50];
			sprintf(error_msg, "Error in start document: %s", error_buf);
			zend_throw_exception(xydiff_exception_ce, error_msg, 0 TSRMLS_CC);
			RETURN_FALSE;
		}

		if (SUCCESS != xydiff_check_libxml_document(node2, &error_buf)) {
			char error_msg [50];
			sprintf(error_msg, "Error in end document: %s", error_buf);
			zend_throw_exception(xydiff_exception_ce, error_msg, 0 TSRMLS_CC);
			RETURN_FALSE;
		}

		xml_object1 = (php_libxml_node_object *) zend_object_store_get_object(doc1 TSRMLS_CC);
		xiddomdocument_sync_with_libxml(xml_object1 TSRMLS_CC);
		xiddoc1 = get_xiddomdocument(xml_object1);

		xml_object2 = (php_libxml_node_object *) zend_object_store_get_object(doc2 TSRMLS_CC);
		xiddomdocument_sync_with_libxml(xml_object2 TSRMLS_CC);
		xiddoc2 = get_xiddomdocument(xml_object2);

		if (xiddoc1 == NULL) {
			zend_throw_exception(xydiff_exception_ce, "Could not process start XIDDOMDocument", 0 TSRMLS_CC);
			RETURN_FALSE;
		}
		if (xiddoc2 == NULL) {
			zend_throw_exception(xydiff_exception_ce, "Could not process end XIDDOMDocument", 0 TSRMLS_CC);
			RETURN_FALSE;
		}

		XyDOMDelta* domDeltaCreate = new XyDOMDelta( xiddoc1, xiddoc2 );
		deltaDoc = domDeltaCreate->createDelta();
		libxml_delta_doc = xid_domdocument_to_libxml_domdocument( deltaDoc TSRMLS_CC );

		if (!libxml_delta_doc)
			RETURN_FALSE;

		// Get the "fromXidMap" attribute in the delta document under first <t> element of the root
		DOMElement *deltaDocRoot = deltaDoc->getDocumentElement();
		DOMElement *tNode = (DOMElement *) deltaDocRoot->getFirstChild();
		XMLCh fromXidMap_attr[11];
		XMLString::transcode("fromXidMap", fromXidMap_attr, 10);
		xidmap = XMLString::transcode( tNode->getAttribute(fromXidMap_attr) );
		if (xidmap != NULL) {
			xiddomdocument_set_xidmap(xml_object2, xidmap TSRMLS_CC);
		}
		
		// Free up memory
		deltaDoc->release();
		delete deltaDoc;
		delete domDeltaCreate;
		XMLString::release(&xidmap);

		DOM_RET_OBJ(rv, (xmlNodePtr) libxml_delta_doc, &ret, NULL);

	}
}