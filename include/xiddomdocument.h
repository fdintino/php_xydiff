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

static void xiddomdocument_object_dtor(void *object TSRMLS_DC);
zend_object_value xiddomdocument_object_create(zend_class_entry *class_type TSRMLS_DC);
void register_xiddomdocument(TSRMLS_D);
void xiddomdocument_sync_with_libxml(php_libxml_node_object *libxml_object TSRMLS_DC, bool hasNewXidmap = false);
XID_DOMDocument * get_xiddomdocument(php_libxml_node_object *object);

int xiddomdocument_set_xidmap(php_libxml_node_object *libxml_object, char *xidmap TSRMLS_DC);

ZEND_METHOD(xiddomdocument, __construct);
ZEND_METHOD(xiddomdocument, __destruct);
ZEND_METHOD(xiddomdocument, getXidMap);
ZEND_METHOD(xiddomdocument, setXidMap);
ZEND_METHOD(xiddomdocument, generateXidTaggedDocument);

xmlDocPtr xid_domdocument_to_libxml_domdocument(XID_DOMDocument *xiddoc TSRMLS_DC);
XID_DOMDocument * libxml_domdocument_to_xid_domdocument(php_libxml_node_object *libxml_doc, bool hasNewXidmap=false TSRMLS_DC);
dom_doc_propsptr dom_get_doc_props(php_libxml_node_object *node);
static void dom_copy_doc_props(php_libxml_ref_obj *source_doc, php_libxml_ref_obj *dest_doc);
int xydiff_check_libxml_document(xmlNode *node, char **error_buf);

#endif