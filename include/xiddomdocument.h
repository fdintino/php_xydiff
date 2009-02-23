/*
 *  xiddomdocument.h
 *  xydiff
 */

#ifndef PHP_XIDDOMDOCUMENT_H
#define PHP_XIDDOMDOCUMENT_H


typedef struct _xiddomdoc_object {
	zend_object std;
	void *ptr;
	php_libxml_ref_obj *document;
	HashTable *prop_handler;
	zend_object_handle handle;
	
	XID_DOMDocument *xiddoc;
	php_libxml_node_object *doc;	
	zend_class_entry *pce_ptr;
} xiddomdoc_object;

static void xiddomdocument_object_dtor(void *object);
static void xiddomdocument_object_clone(void *object, void **object_clone TSRMLS_DC);
zend_object_value xiddomdocument_object_create(zend_class_entry *class_type TSRMLS_DC);
void register_xiddomdocument(TSRMLS_D);
void xiddomdocument_sync_with_libxml(php_libxml_node_object *libxml_object);
XID_DOMDocument * get_xiddomdocument(php_libxml_node_object *object);

ZEND_METHOD(xiddomdocument, __construct);
ZEND_METHOD(xiddomdocument, __destruct);
#endif