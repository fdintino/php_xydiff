/*
 *  xydeltaapply.h
 *  xydiff
 */

#ifndef PHP_XYDELTAAPPLY_H
#define PHP_XYDELTAAPPLY_H

typedef struct _xydelta_object {
	zend_object std;
	php_libxml_node_object *libxml_start_doc;
	xmlDocPtr libxml_delta_doc;
	zval *z_start_doc;
	XID_DOMDocument* xid_delta_doc;
	zend_object_handle handle;
} xydelta_object;

void register_xydelta(TSRMLS_D);
static void xydelta_object_dtor(void *object TSRMLS_DC);
static void xydelta_object_clone(void *object, void **object_clone TSRMLS_DC);
zend_object_value xydelta_object_create(zend_class_entry *class_type TSRMLS_DC);
zend_object_value xydelta_object_store_clone_obj(zval *zobject TSRMLS_DC);

ZEND_METHOD(xydelta, __construct);
ZEND_METHOD(xydelta, setStartDocument);
ZEND_METHOD(xydelta, applyDelta);

#endif