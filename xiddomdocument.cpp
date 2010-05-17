#include "include/php_xydiff.hpp"

#include <assert.h>
#include <sys/types.h>

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
#include "include/xydiff_error_handler.h"

#include <zend_interfaces.h>

XERCES_CPP_NAMESPACE_USE

static zend_class_entry *xiddomdocument_ce;

static zend_function_entry xiddomdocument_methods[] = {
	ZEND_ME(xiddomdocument, __construct, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xiddomdocument, __destruct, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xiddomdocument, getXidMap, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xiddomdocument, setXidMap, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xiddomdocument, generateXidTaggedDocument, NULL, ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};

static zend_object_handlers xiddomdocument_object_handlers;

static zend_class_entry *xydiff_exception_ce;
static zend_class_entry *xy_xml_exception_ce;
static zend_class_entry *xy_dom_exception_ce;

void register_xiddomdocument(TSRMLS_D)
{
	// Load Exception classes
	// XyDiffException
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

	// Initialize XIDDOMDocument class as extension of DOMDocument class
	memcpy(&xiddomdocument_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	zend_class_entry ce;
	zend_class_entry **pce;
	if (zend_hash_find(CG(class_table), "domdocument", sizeof("domdocument"), (void **) &pce) == FAILURE) {
		return;
	}
	INIT_CLASS_ENTRY(ce, "XIDDOMDocument", xiddomdocument_methods);
	ce.create_object = xiddomdocument_object_create;
	xiddomdocument_ce = zend_register_internal_class_ex(&ce, *pce, NULL TSRMLS_CC);
}

static dom_object* xiddomdocument_set_class(zend_class_entry *class_type, zend_bool hash_copy TSRMLS_DC) /* {{{ */
{
	zval *tmp;
	dom_object *intern;
	intern = (dom_object *) emalloc(sizeof(dom_object));
	intern->ptr = NULL;
	intern->prop_handler = NULL;
	intern->document = NULL;
	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	if (hash_copy) {
		zend_hash_copy(intern->std.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
	}
	return intern;
}

zend_object_value xiddomdocument_object_create(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object_value retval;
	dom_object *intern;
	zval *tmp;

	intern = xiddomdocument_set_class(class_type, 1 TSRMLS_CC);
	
	retval.handle = zend_objects_store_put(intern,
										   (zend_objects_store_dtor_t)zend_objects_destroy_object,
										   (zend_objects_free_object_storage_t) xiddomdocument_object_dtor, NULL TSRMLS_CC);
	intern->handle = retval.handle;
	retval.handlers = &xiddomdocument_object_handlers;
	return retval;
}



void xiddomdocument_object_dtor(void *object TSRMLS_DC)
{
	php_libxml_node_object *intern = (php_libxml_node_object *) object;

	XID_DOMDocument *xiddoc = get_xiddomdocument(intern);
	if (xiddoc) {
		xiddoc->release();
		delete xiddoc;
	}

	if (intern->properties != NULL) {
		if (zend_hash_exists(intern->properties, "xiddoc", sizeof("xiddoc"))) {
			xiddoc = get_xiddomdocument(intern);
			if (xiddoc != NULL) {
				xiddoc->release();
				// delete xiddoc;
			}		
			zend_hash_del(intern->properties, "xiddoc", sizeof("xiddoc"));
		}

		if (zend_hash_exists(intern->properties, "xidmap", sizeof("xidmap"))) {
			zval** xidmapval;
			if (zend_hash_find(intern->properties, "xidmap", sizeof("xidmap"), (void**)&xidmapval) == SUCCESS) {
				char *xidmapStr = Z_STRVAL_PP(xidmapval);
				efree(xidmapStr);
				FREE_ZVAL(*xidmapval);
			}
			zend_hash_del(intern->properties, "xidmap", sizeof("xidmap"));
		}
		zend_hash_destroy(intern->properties);
		FREE_HASHTABLE(intern->properties);
	}

	int refcount = php_libxml_decrement_node_ptr(intern TSRMLS_CC);
	php_libxml_decrement_doc_ref(intern TSRMLS_CC);
	efree(object);
}

void xiddomdocument_sync_with_libxml(php_libxml_node_object *libxml_object TSRMLS_DC, bool hasNewXidmap)
{
	XID_DOMDocument *xiddoc = libxml_domdocument_to_xid_domdocument(libxml_object TSRMLS_CC, hasNewXidmap);
	uintptr_t xiddocptr = (uintptr_t) xiddoc;
	uintptr_t *oldxiddocptr;
	if (zend_hash_find(libxml_object->properties, "xiddoc", sizeof("xiddoc"), (void **) &oldxiddocptr ) == SUCCESS) {
		XID_DOMDocument *oldxiddoc = (XID_DOMDocument *) *oldxiddocptr;
		oldxiddoc->release();
		delete oldxiddoc;
	}
	zend_hash_update(libxml_object->properties, "xiddoc", sizeof("xiddoc"), &xiddocptr, sizeof(uintptr_t), NULL);
}

XID_DOMDocument * get_xiddomdocument(php_libxml_node_object *object)
{
	uintptr_t *xiddocptr;
	
	if (object->properties != NULL && zend_hash_find(object->properties, "xiddoc", sizeof("xiddoc"), (void **) &xiddocptr ) == SUCCESS) {
		return (XID_DOMDocument *) *xiddocptr;
	} else {
		return (XID_DOMDocument *) NULL;
	}
}

void propDestructor(void *pElement);
void propDestructor(void *pElement)
{
	pElement = NULL;
}

ZEND_METHOD(xiddomdocument, __destruct)
{
	zval *id;
	php_libxml_node_object *intern;
	XID_DOMDocument *xiddoc;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, xiddomdocument_ce) == FAILURE) {
		return;
	}
	intern = (php_libxml_node_object *) zend_object_store_get_object(id TSRMLS_CC);
	if (intern->properties != NULL) {
		if (zend_hash_exists(intern->properties, "xiddoc", sizeof("xiddoc"))) {
			xiddoc = get_xiddomdocument(intern);
			if (xiddoc != NULL) {
				xiddoc->release();
				// delete xiddoc;
			}		
			zend_hash_del(intern->properties, "xiddoc", sizeof("xiddoc"));
		}

		if (zend_hash_exists(intern->properties, "xidmap", sizeof("xidmap"))) {
			zval** xidmapval;
			if (zend_hash_find(intern->properties, "xidmap", sizeof("xidmap"), (void**)&xidmapval) == SUCCESS) {
				char *xidmapStr = Z_STRVAL_PP(xidmapval);
				efree(xidmapStr);
				FREE_ZVAL(*xidmapval);
			}
			zend_hash_del(intern->properties, "xidmap", sizeof("xidmap"));
		}
		zend_hash_destroy(intern->properties);
		FREE_HASHTABLE(intern->properties);
	}
}

ZEND_METHOD(xiddomdocument, getXidMap)
{
	zval *id;
	php_libxml_node_object *intern;
	XID_DOMDocument *xiddoc;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, xiddomdocument_ce) == FAILURE) {
		return;
	}

	intern = (php_libxml_node_object *) zend_object_store_get_object(id TSRMLS_CC);
	xiddoc = get_xiddomdocument(intern);
	if (xiddoc == NULL) {
		RETURN_STRINGL("", 0, true);
	}
	char *xidmap;
	try {		
		xidmap = new char [ xiddoc->getXidMap().String().length() + 1 ];
		strcpy(xidmap, xiddoc->getXidMap().String().c_str());
	}
	catch( const DOMException& e ) {
		char *exceptionMsg = XMLString::transcode(e.msg);
		zend_throw_exception(xy_xml_exception_ce, exceptionMsg, 0 TSRMLS_CC);
		XMLString::release(&exceptionMsg);
	}	catch ( const DeltaException &e ) {
		zend_throw_exception(xydiff_exception_ce, e.error, 0 TSRMLS_CC);
		RETURN_FALSE;
	}
	catch ( const VersionManagerException &e ) {
		zend_throw_exception(xydiff_exception_ce, strdup((e.context+": " +e.message).c_str()), 0 TSRMLS_CC);
		RETURN_FALSE;
	}
	catch ( ... ) {
		zend_throw_exception(xydiff_exception_ce, "Unexpected exception occurred", 0 TSRMLS_CC);
		RETURN_FALSE;
	}
	RETVAL_STRINGL(xidmap, strlen(xidmap), true);
	delete [] xidmap;
}

ZEND_METHOD(xiddomdocument, setXidMap)
{
	zval *id;
	char *xidmap;
	int xidmap_len;
	php_libxml_node_object *intern;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &id, xiddomdocument_ce, &xidmap, &xidmap_len) == FAILURE) {
		return;
	}
	intern = (php_libxml_node_object *) zend_object_store_get_object(id TSRMLS_CC);
	if (xiddomdocument_set_xidmap(intern, xidmap TSRMLS_CC) == SUCCESS) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}

int xiddomdocument_set_xidmap(php_libxml_node_object *libxml_object, char *xidmap TSRMLS_DC)
{
	XID_DOMDocument *xiddoc = NULL;
	zval *xidmapval;

	xiddoc = get_xiddomdocument(libxml_object);
	if (xiddoc == NULL) {
		xiddomdocument_sync_with_libxml(libxml_object TSRMLS_CC);
		xiddoc = get_xiddomdocument(libxml_object);
	}
	if (xiddoc != NULL) {
		// Remove old xidmap from properties HashTable if it exists
		zval** oldxidmapval;
		if (zend_hash_find(libxml_object->properties, "xidmap", sizeof("xidmap"), (void**)&xidmapval) == SUCCESS) {
			char *oldxidmapStr = Z_STRVAL_P(xidmapval);
			FREE_ZVAL(*oldxidmapval);
			zend_hash_del(libxml_object->properties, "xidmap", strlen("xidmap"));
		}
		MAKE_STD_ZVAL(xidmapval);
		ZVAL_STRING(xidmapval, xidmap, 1);
		ZEND_SET_SYMBOL(libxml_object->properties, "xidmap", xidmapval);
		xiddomdocument_sync_with_libxml(libxml_object TSRMLS_CC, true);
		if (zend_hash_exists(libxml_object->properties, "xidmap", sizeof("xidmap"))) {
			return SUCCESS;
		}
	}
	return FAILURE;
}

ZEND_METHOD(xiddomdocument, generateXidTaggedDocument)
{
	zval *id, *rv;
	php_libxml_node_object *intern;
	XID_DOMDocument *xiddoc;
	int ret;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, xiddomdocument_ce) == FAILURE) {
		return;
	}
	intern = (php_libxml_node_object *) zend_object_store_get_object(id TSRMLS_CC);
	xiddoc = get_xiddomdocument(intern);
	XID_DOMDocument* d = NULL;
	try {
		d = XID_DOMDocument::copy(xiddoc);
		
		DOMNode* root = (DOMNode *) d->getDocumentElement();
		if (root!=NULL) Restricted::XidTagSubtree(d, root);
		xmlDocPtr libxmldoc = xid_domdocument_to_libxml_domdocument(d TSRMLS_CC);

		if (d != NULL) {
			d->release();
			delete d;
		}
		if (!libxmldoc)
			RETURN_FALSE;
		DOM_RET_OBJ(rv, (xmlNodePtr) libxmldoc, &ret, NULL);
		
	}
	catch( const DOMException& e ) {
		char *exceptionMsg = XMLString::transcode(e.msg);
		zend_throw_exception(xy_xml_exception_ce, exceptionMsg, 0 TSRMLS_CC);
		XMLString::release(&exceptionMsg);
	}	catch ( const DeltaException &e ) {
		zend_throw_exception(xydiff_exception_ce, e.error, 0 TSRMLS_CC);
	}
	catch( const VersionManagerException &e ) {
		zend_throw_exception(xydiff_exception_ce, estrdup((e.context+": " +e.message).c_str()), 0 TSRMLS_CC);
	}
	catch ( ... ) {
		zend_throw_exception(xydiff_exception_ce, "Unexpected exception occurred", 0 TSRMLS_CC);
	}
}

ZEND_METHOD(xiddomdocument, __construct)
{
	zval *id;
	dom_object *intern;
	zend_class_entry **pce;
	zend_class_entry *ce = xiddomdocument_ce;
	
	char *encoding, *version = NULL;
	int encoding_len = 0, version_len = 0, refcount;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O|ss", &id, xiddomdocument_ce,
									 &version, &version_len, &encoding, &encoding_len) == FAILURE) {
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
		zval *encoding_str;
		zval *version_str;
		if (version_len > 0) {
			MAKE_STD_ZVAL(version_str);
			ZVAL_STRING(version_str, version, 1);		
		}
		if (encoding_len > 0) {
			MAKE_STD_ZVAL(encoding_str);
			ZVAL_STRING(encoding_str, encoding, 1);
		}
		if (encoding_len > 0) {
			zend_call_method_with_2_params(&self, ce, &ctor, ZEND_CONSTRUCTOR_FUNC_NAME, NULL, version_str, encoding_str);
			FREE_ZVAL(encoding_str);
			FREE_ZVAL(version_str);
		} else if (version_len > 0) {
			zend_call_method_with_1_params(&self, ce, &ctor, ZEND_CONSTRUCTOR_FUNC_NAME, NULL, version_str);
			FREE_ZVAL(version_str);
		} else {
			zend_call_method_with_0_params(&self, ce, &ctor, ZEND_CONSTRUCTOR_FUNC_NAME, NULL);
		}
	}
}

xmlDocPtr xid_domdocument_to_libxml_domdocument(XID_DOMDocument *xiddoc TSRMLS_DC)
{
	static const XMLCh gLS[] = { chLatin_L, chLatin_S, chNull };
	DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(gLS);
	DOMLSSerializer* theSerializer = ((DOMImplementationLS*)impl)->createLSSerializer();
	DOMLSOutput *theOutput = ((DOMImplementationLS*)impl)->createLSOutput();
	
	XMLFormatTarget *myFormatTarget = new MemBufFormatTarget();
	theOutput->setByteStream(myFormatTarget);
	
	try {
		if (theSerializer->getDomConfig()->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true))
			theSerializer->getDomConfig()->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);
		if (theSerializer->getDomConfig()->canSetParameter(XMLUni::fgDOMXMLDeclaration, true)) 
		    theSerializer->getDomConfig()->setParameter(XMLUni::fgDOMXMLDeclaration, true);		
		theSerializer->write((DOMDocument*)xiddoc, theOutput);
	}
	catch (const XMLException& e) {
		char *exceptionMsg = XMLString::transcode(e.getMessage());
		zend_throw_exception(xy_dom_exception_ce, exceptionMsg, 0 TSRMLS_CC);
		XMLString::release(&exceptionMsg);
	}
	catch( const DOMException& e ) {
		char *exceptionMsg = XMLString::transcode(e.msg);
		zend_throw_exception(xy_xml_exception_ce, exceptionMsg, 0 TSRMLS_CC);
		XMLString::release(&exceptionMsg);
	}
	catch (...) {
		zend_throw_exception(xydiff_exception_ce, "Unexpected exception", 0 TSRMLS_CC);
	}
	
	char* theXMLString_Encoded = (char*) ((MemBufFormatTarget*)myFormatTarget)->getRawBuffer();
	int xmlLen = (int) ((MemBufFormatTarget*)myFormatTarget)->getLen();
	
	char *xmlString = (char *) emalloc(sizeof(char)*xmlLen+1);
	strncpy (xmlString, theXMLString_Encoded, xmlLen);
	xmlString[xmlLen] = '\0';
	
	theOutput->release();
	theSerializer->release();
	delete myFormatTarget;
	
	xmlDocPtr newdoc;
	xmlParserCtxtPtr ctxt = NULL;
	
	ctxt = xmlCreateDocParserCtxt((xmlChar *)xmlString);
	if (ctxt == NULL) {
		newdoc = NULL;
	}
	xmlParseDocument(ctxt);
	if (ctxt->wellFormed) {
		newdoc = ctxt->myDoc;
	} else {
		xmlFreeDoc(ctxt->myDoc);
		ctxt->myDoc = NULL;
	}
	xmlFreeParserCtxt(ctxt);
	efree(xmlString);
	return newdoc;
}

XID_DOMDocument * libxml_domdocument_to_xid_domdocument(php_libxml_node_object *libxml_obj TSRMLS_DC, bool hasNewXidmap)
{
	xmlChar *mem = NULL;
	int size = 0;
	DOMDocument *xiddoc;
	dom_doc_propsptr doc_props;
	int format = 0;
	xmlDocPtr docp;
	
	docp = (xmlDocPtr) libxml_obj->document->ptr;
	doc_props = dom_get_doc_props(libxml_obj);
	format = doc_props->formatoutput;
	xmlDocDumpFormatMemory(docp, &mem, &size, format);
	
	const char *xmlString = (const char *)mem;
	static const XMLCh gLS[] = { chLatin_L, chLatin_S, chNull };
	DOMImplementation *impl = NULL;
	DOMLSParser *theParser = NULL;
	DOMDocument *domdoc;
	try {
		
		impl = DOMImplementationRegistry::getDOMImplementation(gLS);
		theParser = ((DOMImplementationLS*)impl)->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
		theParser->getDomConfig()->setParameter(XMLUni::fgXercesUserAdoptsDOMDocument, true);
		DOMErrorHandler * handler = new xydiffPHPParseHandler();
		MemBufInputSource *memIS = new MemBufInputSource((const XMLByte *)xmlString, strlen(xmlString), "test", false);
		Wrapper4InputSource *wrapper = new Wrapper4InputSource(memIS, false);		
		theParser->getDomConfig()->setParameter(XMLUni::fgDOMErrorHandler, handler);
		xiddoc = theParser->parse((DOMLSInput *) wrapper);

		wrapper->release();
		theParser->release();
		delete memIS;
		delete handler;
	}
	catch (const XMLException& e) {
		char *exceptionMsg = XMLString::transcode(e.getMessage());
		zend_throw_exception(xy_dom_exception_ce, exceptionMsg, 0 TSRMLS_CC);
		XMLString::release(&exceptionMsg);
	}
	catch( const DOMException& e ) {
		char *exceptionMsg = XMLString::transcode(e.msg);
		zend_throw_exception(xy_xml_exception_ce, exceptionMsg, 0 TSRMLS_CC);
		XMLString::release(&exceptionMsg);
	}
	catch (...) {
		zend_throw_exception(xydiff_exception_ce, "Unexpected exception during XML parsing", 0 TSRMLS_CC);
	}
	if (size) {
		xmlFree(mem);
	}

	// Try creating with xidmap if the user has defined one
	char*  xidmapStr = NULL;
	zval** xidmapval;
	XID_DOMDocument* xiddomdoc = NULL;
	char *errormsg = NULL;
	if (libxml_obj->properties == NULL) {
		// Set up properties hash table
		ALLOC_HASHTABLE(libxml_obj->properties);
		if (zend_hash_init(libxml_obj->properties, 50, NULL, NULL, 0) == FAILURE) {
			FREE_HASHTABLE(libxml_obj->properties);
		}
	}
	if (zend_hash_find(libxml_obj->properties, "xidmap", sizeof("xidmap"), (void**)&xidmapval) == SUCCESS) {
		xidmapStr = Z_STRVAL_PP(xidmapval);
		if (xidmapStr != NULL && strlen(xidmapStr) > 0) {
			try {
				xiddomdoc = new XID_DOMDocument(xiddoc, xidmapStr, true);
			}
			catch ( const DeltaException &e ) {
				errormsg = estrdup(e.error);
				delete xiddomdoc;
				xiddomdoc = NULL;
			} catch ( const XIDMapException &e ) {
				errormsg = estrdup((e.context+": " +e.message).c_str());
				delete xiddomdoc;
				xiddomdoc = NULL;
			} catch ( ... ) {
				errormsg = estrdup("Unexpected exception while setting XidMap");
				delete xiddomdoc;
				xiddomdoc = NULL;
			}
		}
	}
	
	
	// Create XID_DOMDocument without a user-defined xidmap
	if (xiddomdoc == NULL) {
		try {
			xiddomdoc = new XID_DOMDocument(xiddoc, NULL, true);
			// If we have a xidmapStr this means that setting the XID_DOMDocument with
			// the new xidmap value failed
			if (xidmapStr != NULL && strlen(xidmapStr) > 0) {
				// If there is already a XID_DOMDocument
				if (hasNewXidmap) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING,
									 "Could not set xidmap, errormsg=%s", errormsg);					
				} else {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, 
									 "XidMap value no longer applicable, resetting to default; errormsg=%s", errormsg);
				}
				// Remove old xidmap value, free memory
				efree(xidmapStr);
				FREE_ZVAL(*xidmapval);
				zend_hash_del(libxml_obj->properties, "xidmap", sizeof("xidmap"));
			}
		}
		catch ( const DeltaException &e ) {
			zend_throw_exception(xydiff_exception_ce, e.error, 0 TSRMLS_CC);
		}
		catch ( const XIDMapException &e ) {
			zend_throw_exception(xydiff_exception_ce, strdup((e.context+": " +e.message).c_str()), 0 TSRMLS_CC);
		}
		catch ( ... ) {
			zend_throw_exception(xydiff_exception_ce, "Unexpected exception while creating XIDDOMDocument", 0 TSRMLS_CC);
		}
	}
	if (errormsg != NULL) {
		efree(errormsg);
	}

	return xiddomdoc;
}


dom_doc_propsptr dom_get_doc_props(php_libxml_node_object *node)
{
	dom_doc_propsptr doc_props;
	php_libxml_ref_obj *document = node->document;
	
	if (document && document->doc_props) {
		return document->doc_props;
	} else {
		doc_props = (libxml_doc_props*) emalloc(sizeof(libxml_doc_props));
		doc_props->formatoutput = 0;
		doc_props->validateonparse = 0;
		doc_props->resolveexternals = 0;
		doc_props->preservewhitespace = 1;
		doc_props->substituteentities = 0;
		doc_props->stricterror = 1;
		doc_props->recover = 0;
		doc_props->classmap = NULL;
		if (document) {
			document->doc_props = doc_props;
		}
		return doc_props;
	}
}

static void dom_copy_doc_props(php_libxml_ref_obj *source_doc, php_libxml_ref_obj *dest_doc)
{
	dom_doc_propsptr source, dest;
	
	if (source_doc && dest_doc) {
		
		source = dom_get_doc_props((php_libxml_node_object *)source_doc);
		dest = dom_get_doc_props((php_libxml_node_object *)dest_doc);
		
		dest->formatoutput = source->formatoutput;
		dest->validateonparse = source->validateonparse;
		dest->resolveexternals = source->resolveexternals;
		dest->preservewhitespace = source->preservewhitespace;
		dest->substituteentities = source->substituteentities;
		dest->stricterror = source->stricterror;
		dest->recover = source->recover;
		if (source->classmap) {
			ALLOC_HASHTABLE(dest->classmap);
			zend_hash_init(dest->classmap, 0, NULL, NULL, 0);
			zend_hash_copy(dest->classmap, source->classmap, NULL, NULL, sizeof(zend_class_entry *));
		}
		
	}
}

// Do some sanity checks on a DOMDocument that is passed
int xydiff_check_libxml_document(xmlNode *node, char **error_buf) {
	xmlDocPtr doc = NULL;
	
	if (node == NULL)
		return FAILURE;
	if (node) {
		if (node->doc == NULL) {
			*error_buf = "Imported Node must have associated Document";
			return FAILURE;
		}
		if (node->type == XML_DOCUMENT_NODE || node->type == XML_HTML_DOCUMENT_NODE) {
			node = xmlDocGetRootElement((xmlDocPtr) node);
		}
		if (node == NULL) {
			*error_buf = "Imported document has empty DOM tree";
			return FAILURE;
		}
		doc = node->doc;
	}
	if (doc == NULL) {
		*error_buf = "Invalid Document";
		return FAILURE;
	} else {
		return SUCCESS;
	}
}

zval *xiddomdocument_create(XID_DOMDocument *xiddoc, int *found, zval *in, zval *return_value TSRMLS_DC) {
	dom_object *retdoc = NULL;
	dom_object *intern = NULL;
	zval *wrapper = NULL;
	xmlNodePtr libxml_result_doc = NULL;
	php_libxml_node_object *xml_object;
	*found = 0;
	char *xidmap = NULL;
	uintptr_t xiddocptr = NULL;
	
	libxml_result_doc = (xmlNodePtr) xid_domdocument_to_libxml_domdocument( xiddoc TSRMLS_CC );

	wrapper = return_value;
	object_init_ex(wrapper, xiddomdocument_ce);

	intern = (dom_object *) zend_objects_get_address(wrapper TSRMLS_CC);

	if (libxml_result_doc->doc != NULL) {
		php_libxml_increment_doc_ref((php_libxml_node_object *)intern, libxml_result_doc->doc TSRMLS_CC);
	}

	php_libxml_increment_node_ptr((php_libxml_node_object *)intern, libxml_result_doc, (void *)intern TSRMLS_CC);

	xml_object = (php_libxml_node_object *) intern;
	if (xml_object->properties == NULL) {
		ALLOC_HASHTABLE(xml_object->properties);
		if (zend_hash_init(xml_object->properties, 50, NULL, NULL, 0) == FAILURE) {
			FREE_HASHTABLE(xml_object->properties);
			ZVAL_NULL(wrapper);
			return wrapper;
		}			
	}
	xiddocptr = (uintptr_t) xiddoc;
	zend_hash_update(xml_object->properties, "xiddoc", sizeof("xiddoc"), &xiddocptr, sizeof(uintptr_t), NULL);
	
	try {		
		xidmap = new char [ xiddoc->getXidMap().String().length() + 1 ];
		strcpy(xidmap, xiddoc->getXidMap().String().c_str());
	}
	catch( const DOMException& e ) {
		char *exceptionMsg = XMLString::transcode(e.msg);
		zend_throw_exception(xy_xml_exception_ce, exceptionMsg, 0 TSRMLS_CC);
		XMLString::release(&exceptionMsg);
	}	catch ( const DeltaException &e ) {
		zend_throw_exception(xydiff_exception_ce, e.error, 0 TSRMLS_CC);
		ZVAL_NULL(wrapper);
		return wrapper;
	}
	catch ( const VersionManagerException &e ) {
		zend_throw_exception(xydiff_exception_ce, strdup((e.context+": " +e.message).c_str()), 0 TSRMLS_CC);
		ZVAL_NULL(wrapper);
		return wrapper;
	}
	catch ( ... ) {
		zend_throw_exception(xydiff_exception_ce, "Unexpected exception occurred", 0 TSRMLS_CC);
		ZVAL_NULL(wrapper);
		return wrapper;
	}
	xiddomdocument_set_xidmap(xml_object, xidmap TSRMLS_CC);

	return (wrapper);
}