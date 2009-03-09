#ifndef PHP_XYDIFFPROCESSOR_H
#define PHP_XYDIFFPROCESSOR_H

typedef struct _xydiff_object {
	zend_object std;
	XID_DOMDocument *xiddoc1;
	XID_DOMDocument *xiddoc2;
	php_libxml_node_object *doc2;
	xmlDocPtr libxml_delta_doc;
	zend_object_handle handle;
} xydiff_object;

static void xydiff_object_dtor(void *object);
static void xydiff_object_clone(void *object, void **object_clone TSRMLS_DC);
zend_object_value xydiff_object_create(zend_class_entry *class_type TSRMLS_DC);
void register_xydiff(TSRMLS_D);
xercesc::DOMDocument * string_to_xid_domdocument(const char *string);

ZEND_METHOD(xydiff, __construct);
ZEND_METHOD(xydiff, setStartDocument);
ZEND_METHOD(xydiff, setEndDocument);
ZEND_METHOD(xydiff, createDelta);

#endif