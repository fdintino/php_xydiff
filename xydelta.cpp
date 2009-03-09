/*
 *  xydelta.cpp
 *  xydiff
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 */

#include "include/php_xydiff.hpp"
#include "include/xydelta.h"
#include <zend_interfaces.h>

#define XYDELTA_CLASS_NAME "XyDelta"

XERCES_CPP_NAMESPACE_USE

static zend_class_entry *xydelta_ce;

static zend_function_entry xydelta_methods[] = {
ZEND_ME(xydelta, __construct, NULL, ZEND_ACC_PUBLIC)
ZEND_ME(xydelta, setStartDocument, NULL, ZEND_ACC_PUBLIC)
ZEND_ME(xydelta, applyDelta, NULL, ZEND_ACC_PUBLIC)
{ NULL, NULL, NULL }
};
static zend_object_handlers xydelta_object_handlers;

static zend_class_entry *xydiff_exception_ce;

void register_xydelta(TSRMLS_D)
{
	memcpy(&xydelta_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, XYDELTA_CLASS_NAME, xydelta_methods);
	ce.create_object = xydelta_object_create;
	xydelta_ce = zend_register_internal_class(&ce TSRMLS_CC);
	zend_class_entry **xydiff_exception_ce_ptr;
	if (zend_hash_find(CG(class_table), "xydiffexception", sizeof("xydiffexception"), (void **) &xydiff_exception_ce_ptr) == FAILURE) {
		xydiff_exception_ce = zend_exception_get_default(TSRMLS_C);
	} else {
		xydiff_exception_ce = *xydiff_exception_ce_ptr;
	}
}

static void xydelta_object_dtor(void *object TSRMLS_DC)
{
	xydelta_object *intern = (xydelta_object *)object;
	zend_object_std_dtor(&intern->std TSRMLS_CC);
	efree(object);
}

zend_object_value xydelta_object_create(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object_value retval;
	xydelta_object *intern;
	zval *tmp;
	
	intern = (xydelta_object *) emalloc(sizeof(xydelta_object));
	memset(intern, 0, sizeof(xydelta_object));
	intern->libxml_delta_doc = NULL;
	intern->xid_delta_doc = NULL;
	
	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	zend_hash_copy(intern->std.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
	
	retval.handle = zend_objects_store_put(intern,
										   (zend_objects_store_dtor_t)zend_objects_destroy_object,
										   (zend_objects_free_object_storage_t) xydelta_object_dtor, NULL TSRMLS_CC);
	intern->handle = retval.handle;
	retval.handlers = &xydelta_object_handlers;
	return retval;
	
}

static void xydelta_object_clone(void *object, void **object_clone TSRMLS_DC)
{
	xydelta_object *intern = (xydelta_object *) object;
	xydelta_object **intern_clone = (xydelta_object **) object_clone;
	
	*intern_clone = (xydelta_object *) emalloc(sizeof(xydelta_object));
	if (intern->xid_delta_doc != NULL) {
		(*intern_clone)->xid_delta_doc = XID_DOMDocument::copy(intern->xid_delta_doc, 1);
	} else {
		(*intern_clone)->xid_delta_doc = NULL;
	}
}

ZEND_METHOD(xydelta, __construct)
{
	
}

ZEND_METHOD(xydelta, setStartDocument)
{
	
}

ZEND_METHOD(xydelta, applyDelta)
{
	
}