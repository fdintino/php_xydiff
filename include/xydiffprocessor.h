#ifndef PHP_XYDIFFPROCESSOR_H
#define PHP_XYDIFFPROCESSOR_H

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