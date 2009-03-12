/*
 *  xydelta.cpp
 *  xydiff
 */

#include "include/php_xydiff.hpp"
#include "include/xydelta.h"
#include "include/xiddomdocument.h"
#include <zend_interfaces.h>

#include "xercesc/dom/DOMElement.hpp"
#include "xercesc/dom/DOMNode.hpp"

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
static zend_class_entry *xy_xml_exception_ce;
static zend_class_entry *xy_dom_exception_ce;

void register_xydelta(TSRMLS_D) {
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
	// XyDomException
	zend_class_entry **xy_dom_exception_ce_ptr;
	if (zend_hash_find(CG(class_table), "xydomexception", sizeof("xydomexception"), (void **) &xy_dom_exception_ce_ptr) == FAILURE) {
		xy_dom_exception_ce = zend_exception_get_default(TSRMLS_C);
	} else {
		xy_dom_exception_ce = *xy_dom_exception_ce_ptr;
	}
	// XyXMLException
	zend_class_entry **xy_xml_exception_ce_ptr;
	if (zend_hash_find(CG(class_table), "xyxmlexception", sizeof("xyxmlexception"), (void **) &xy_xml_exception_ce_ptr) == FAILURE) {
		xy_xml_exception_ce = zend_exception_get_default(TSRMLS_C);
	} else {
		xy_xml_exception_ce = *xy_xml_exception_ce_ptr;
	}	
}

static void xydelta_object_dtor(void *object TSRMLS_DC) {
	xydelta_object *intern = (xydelta_object *)object;
	
	if (zend_hash_exists(intern->libxml_start_doc->properties, "xiddoc", sizeof("xiddoc"))) {
		XID_DOMDocument *xiddoc = get_xiddomdocument(intern->libxml_start_doc);
		if (xiddoc != NULL) {
			xiddoc->release();
			delete xiddoc;
		}		
		zend_hash_del(intern->libxml_start_doc->properties, "xiddoc", sizeof("xiddoc"));
	}
	if (zend_hash_exists(intern->libxml_start_doc->properties, "xidmap", sizeof("xidmap")) == 1) {
		zval** xidmapval;
		if (zend_hash_find(intern->libxml_start_doc->properties, "xidmap", sizeof("xidmap"), (void**)&xidmapval) == SUCCESS) {
			char *xidmapStr = Z_STRVAL_PP(xidmapval);
			efree(xidmapStr);
			FREE_ZVAL(*xidmapval);
		}
		zend_hash_del(intern->libxml_start_doc->properties, "xidmap", sizeof("xidmap"));
	}
	zend_hash_destroy(intern->libxml_start_doc->properties);
	FREE_HASHTABLE(intern->libxml_start_doc->properties);

	// Free the start doc
	int refcount = php_libxml_decrement_node_ptr((php_libxml_node_object *)intern->libxml_start_doc TSRMLS_CC);
	php_libxml_decrement_doc_ref((php_libxml_node_object *)intern->libxml_start_doc TSRMLS_CC);

	zend_object_std_dtor(&intern->std TSRMLS_CC);
	efree(object);
}

zend_object_value xydelta_object_create(zend_class_entry *class_type TSRMLS_DC) {
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

static void xydelta_object_clone(void *object, void **object_clone TSRMLS_DC) {
	xydelta_object *intern = (xydelta_object *) object;
	xydelta_object **intern_clone = (xydelta_object **) object_clone;
	
	*intern_clone = (xydelta_object *) emalloc(sizeof(xydelta_object));
	if (intern->xid_delta_doc != NULL) {
		(*intern_clone)->xid_delta_doc = XID_DOMDocument::copy(intern->xid_delta_doc, 1);
	} else {
		(*intern_clone)->xid_delta_doc = NULL;
	}
}

ZEND_METHOD(xydelta, __construct) {
	
}

ZEND_METHOD(xydelta, setStartDocument) {
	zval *id, *doc = NULL;
	xydelta_object *intern;
	xmlNode *node = NULL;
	php_libxml_node_object *xml_object;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oo", &id, xydelta_ce, &doc) == FAILURE) {
		RETURN_FALSE;
	}

	intern = (xydelta_object *)zend_object_store_get_object(id TSRMLS_CC);
	if (intern != NULL) {	
		node = php_libxml_import_node(doc TSRMLS_CC);
		
		// Do some sanity checks on the DOMDocument that was passed
		char *error_buf;
		
		if (SUCCESS != xydiff_check_libxml_document(node, &error_buf)) {
			char error_msg [50];
			sprintf(error_msg, "Error in start document: %s", error_buf);
			zend_throw_exception(xydiff_exception_ce, error_msg, 0 TSRMLS_CC);
			delete [] error_buf;
			RETURN_FALSE;
		}

		// Clone the object we've been passed
		_zend_object_store_bucket::_store_bucket::_store_object *doc_obj;
		zend_object_handle handle = Z_OBJ_HANDLE_P(doc);
		
		doc_obj = &EG(objects_store).object_buckets[handle].bucket.obj;
		
		zend_object_handlers *handlers = doc->value.obj.handlers;

		// Set up zval to hold our cloned XIDDOMDocument
		MAKE_STD_ZVAL(intern->z_start_doc);
		Z_TYPE_P(intern->z_start_doc) = IS_OBJECT;
		intern->z_start_doc->value.obj = handlers->clone_obj(doc);


		// Set the libxml_start_doc to the newly created php_libxml_node_object
		intern->libxml_start_doc = (php_libxml_node_object *) zend_object_store_get_object(intern->z_start_doc TSRMLS_CC);

		// Set up properties hash table
		ALLOC_HASHTABLE(intern->libxml_start_doc->properties);
		if (zend_hash_init(intern->libxml_start_doc->properties, 50, NULL, NULL, 0) == FAILURE) {
			FREE_HASHTABLE(intern->libxml_start_doc->properties);
			return;
		}

		xiddomdocument_sync_with_libxml(intern->libxml_start_doc TSRMLS_CC);
	}
}

ZEND_METHOD(xydelta, applyDelta) {
	zval *id = NULL;
	xydelta_object *intern;
	zval *deltadoc = NULL;
	xmlNode *node = NULL;
	php_libxml_node_object *delta_libxml_obj;
	XID_DOMDocument *delta_xiddoc = NULL;
	xmlDocPtr libxml_result_doc = NULL;
	char *xidmap = NULL;
	zval *rv = NULL;
	int ret;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oo", &id, xydelta_ce, &deltadoc) == FAILURE) {
		RETURN_FALSE;
	}

	intern = (xydelta_object *)zend_object_store_get_object(id TSRMLS_CC);
	if (intern != NULL) {
		node = php_libxml_import_node(deltadoc TSRMLS_CC);

		// Do some sanity checks on the DOMDocument that was passed
		char *error_buf;
		
		if (SUCCESS != xydiff_check_libxml_document(node, &error_buf)) {
			char error_msg [50];
			sprintf(error_msg, "Error in delta document: %s", error_buf);
			zend_throw_exception(xydiff_exception_ce, error_msg, 0 TSRMLS_CC);
			RETURN_FALSE;
		}

		delta_libxml_obj = (php_libxml_node_object *) zend_object_store_get_object(deltadoc TSRMLS_CC);
		delta_xiddoc = libxml_domdocument_to_xid_domdocument(delta_libxml_obj);
		xiddomdocument_sync_with_libxml(intern->libxml_start_doc TSRMLS_CC);
		XID_DOMDocument *start_xiddoc = get_xiddomdocument(intern->libxml_start_doc);


		DOMDocument *resultDoc = NULL;
		try {
			DOMElement *deltaDocRoot = delta_xiddoc->getDocumentElement();
			DOMNode *tNode = deltaDocRoot->getFirstChild();
			start_xiddoc->addXidMap(NULL);
			resultDoc = XyDelta::ApplyDelta(start_xiddoc, tNode, false );
		}
		catch ( const DOMException &e ) {
			if (delta_xiddoc != NULL) {
				delete delta_xiddoc;
			}
			zend_throw_exception(xy_dom_exception_ce, XMLString::transcode(e.msg), 0 TSRMLS_CC);
		}
		catch ( const DeltaException &e ) {
			zend_throw_exception(xydiff_exception_ce, e.error, 0 TSRMLS_CC);
		}
		catch ( const VersionManagerException &e ) {
			zend_throw_exception(xydiff_exception_ce, strdup((e.context+": " +e.message).c_str()), 0 TSRMLS_CC);
		}
		catch(const XMLException &e) {
			zend_throw_exception(xy_xml_exception_ce, XMLString::transcode(e.getMessage()), 0 TSRMLS_CC);
		}		
		catch ( ... ) {
			zend_throw_exception(xydiff_exception_ce, "Unexpected exception occurred", 0 TSRMLS_CC);
		}
		if (resultDoc != NULL) {
			XID_DOMDocument *resultXidDoc = new XID_DOMDocument(resultDoc);
			libxml_result_doc = xid_domdocument_to_libxml_domdocument( resultXidDoc TSRMLS_CC );

			// Release resources (free memory) associated with the XID_DOMDocument of the result
			if (resultXidDoc != NULL) {
				resultXidDoc->release();
				delete resultXidDoc;
			}
			DOM_RET_OBJ(rv, (xmlNodePtr) libxml_result_doc, &ret, NULL);
		}
		if (delta_xiddoc != NULL) {
			delete delta_xiddoc;
		}
		// Free up memory
		if (resultDoc != NULL) {
			resultDoc->release();
		}
		if (!libxml_result_doc)
			RETURN_FALSE;
	}
}