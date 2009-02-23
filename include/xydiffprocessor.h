static void xydiff_object_dtor(void *object);
static void xydiff_object_clone(void *object, void **object_clone TSRMLS_DC);
zend_object_value xydiff_object_create(zend_class_entry *class_type TSRMLS_DC);
void register_xydiff(TSRMLS_D);
XID_DOMDocument * libxml_domdocument_to_xid_domdocument(dom_object *libxml_doc);
static xmlDocPtr xid_domdocument_to_libxml_domdocument(XID_DOMDocument *xiddoc);
XID_DOMDocument * libxml_domdocument_to_xid_domdocument(php_libxml_node_object *libxml_doc);
xercesc::DOMDocument * string_to_xid_domdocument(const char *string);
