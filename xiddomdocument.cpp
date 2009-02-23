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
static HashTable xiddomdoc_prop_info;

static zend_function_entry xiddomdocument_methods[] = {
	ZEND_ME(xiddomdocument, __construct, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xiddomdocument, __destruct, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xiddomdocument, loadXML, NULL, ZEND_ACC_PUBLIC)
//ZEND_ME(xiddomdocument, diffXML, NULL, ZEND_ACC_PUBLIC)
{ NULL, NULL, NULL }
};

static zend_object_handlers xiddomdocument_object_handlers;
static void xiddomdoc_prop_info_destroy(void *pi_hash);
static void xiddomdoc_prop_info_destroy(void *pi_hash)
{
//	HashTable *pih = (HashTable *) pi_hash;
//	zend_hash_destroy(pih);
}

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
//	intern->pce_ptr = NULL;
//	intern->doc = NULL;
//	intern->xiddoc = NULL;
	
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
	zend_hash_init_ex(&xiddomdoc_prop_info, 50, NULL, (dtor_func_t) xiddomdoc_prop_info_destroy, 1, 0);	
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
	zend_hash_add(libxml_object->properties, "xiddoc", sizeof("xiddoc"), &xiddocptr, sizeof(uintptr_t), NULL);		
}

XID_DOMDocument * get_xiddomdocument(php_libxml_node_object *object)
{
	uintptr_t *xiddocptr;
	zend_hash_find(object->properties, "xiddoc", sizeof("xiddoc"), (void **) &xiddocptr );
	if (xiddocptr != NULL) {
		return (XID_DOMDocument *) xiddocptr[0];
	}
}

ZEND_METHOD(xiddomdocument, loadXML)
{ }

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
