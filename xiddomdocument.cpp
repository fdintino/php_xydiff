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
#include "include/xydiff_error_handler.h"

#include <zend_interfaces.h>

XERCES_CPP_NAMESPACE_USE

static zend_class_entry *xiddomdocument_class_entry;

static zend_function_entry xiddomdocument_methods[] = {
	ZEND_ME(xiddomdocument, __construct, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xiddomdocument, __destruct, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xiddomdocument, getXidMap, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xiddomdocument, setXidMap, NULL, ZEND_ACC_PUBLIC)
	ZEND_ME(xiddomdocument, generateXidTaggedDocument, NULL, ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};

static zend_object_handlers xiddomdocument_object_handlers;

PHPAPI zend_class_entry *xydiff_exception_ce;
PHPAPI zend_class_entry *xy_xml_exception_ce;
PHPAPI zend_class_entry *xy_dom_exception_ce;


void register_xiddomdocument(TSRMLS_DC)
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
	xiddomdocument_class_entry = zend_register_internal_class_ex(&ce, *pce, NULL TSRMLS_CC);
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

// This doesn't get called?
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
	zend_hash_update(libxml_object->properties, "xiddoc", sizeof("xiddoc"), &xiddocptr, sizeof(uintptr_t), NULL);		
}

XID_DOMDocument * get_xiddomdocument(php_libxml_node_object *object)
{
	uintptr_t *xiddocptr;
	zend_hash_find(object->properties, "xiddoc", sizeof("xiddoc"), (void **) &xiddocptr );
	if (xiddocptr != NULL) {
		return (XID_DOMDocument *) *xiddocptr;
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
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, xiddomdocument_class_entry) == FAILURE) {
		return;
	}
	intern = (php_libxml_node_object *) zend_object_store_get_object(id TSRMLS_CC);
	xiddoc = get_xiddomdocument(intern);
	if (xiddoc != NULL) {
		xiddoc->release();
	}
	zend_hash_destroy(intern->properties);
	FREE_HASHTABLE(intern->properties);
}

ZEND_METHOD(xiddomdocument, getXidMap)
{
	zval *id;
	php_libxml_node_object *intern;
	XID_DOMDocument *xiddoc;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, xiddomdocument_class_entry) == FAILURE) {
		return;
	}

	intern = (php_libxml_node_object *) zend_object_store_get_object(id TSRMLS_CC);
	xiddoc = get_xiddomdocument(intern);
	const char *xidmap = xiddoc->getXidMap().String().c_str();
	RETVAL_STRINGL(xidmap, strlen(xidmap), true);
}

ZEND_METHOD(xiddomdocument, setXidMap)
{
	zval *id;
	char *xidmap;
	int xidmap_len;
	php_libxml_node_object *intern;
	XID_DOMDocument *xiddoc;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &id, xiddomdocument_class_entry, &xidmap, &xidmap_len) == FAILURE) {
		return;
	}

	intern = (php_libxml_node_object *) zend_object_store_get_object(id TSRMLS_CC);
	xiddoc = get_xiddomdocument(intern);
	try {
		xiddoc->addXidMap(xidmap);
	} catch( const DeltaException &e ) {
		zend_throw_exception(xydiff_exception_ce, e.error, 0 TSRMLS_CC);
	} catch (const XIDMapException &e ) {
		zend_throw_exception(xydiff_exception_ce, strdup((e.context+": " +e.message).c_str()), 0 TSRMLS_CC);
	} catch ( ... ) {
		zend_throw_exception(xydiff_exception_ce, "Unexpected exception while setting XidMap", 0 TSRMLS_CC);
	}
}

ZEND_METHOD(xiddomdocument, generateXidTaggedDocument)
{
	zval *id, *rv;
	php_libxml_node_object *intern;
	XID_DOMDocument *xiddoc;
	int ret;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, xiddomdocument_class_entry) == FAILURE) {
		return;
	}
	intern = (php_libxml_node_object *) zend_object_store_get_object(id TSRMLS_CC);
	xiddoc = get_xiddomdocument(intern);
	try {
		XID_DOMDocument* d = XID_DOMDocument::copy(xiddoc);
		
		DOMNode* root = (DOMNode *) d->getDocumentElement();
		if (root!=NULL) Restricted::XidTagSubtree(d, root);
		xmlDocPtr libxmldoc = xid_domdocument_to_libxml_domdocument(d);
		
		//d->release();
		//delete d;
		if (!libxmldoc)
			RETURN_FALSE;
		DOM_RET_OBJ(rv, (xmlNodePtr) libxmldoc, &ret, NULL);
		
	}
	catch( const VersionManagerException &e ) {
		zend_throw_exception(xydiff_exception_ce, strdup((e.context+": " +e.message).c_str()), 0 TSRMLS_CC);
	}
	catch( const DOMException &e ) {
		zend_throw_exception(xy_dom_exception_ce, XMLString::transcode(e.msg), 0 TSRMLS_CC);
	}
	catch ( const DeltaException &e ) {
		zend_throw_exception(xydiff_exception_ce, e.error, 0 TSRMLS_CC);
	}
	catch ( ... ) {
		zend_throw_exception(xydiff_exception_ce, "Unknown error", 0 TSRMLS_CC);
	}
}

ZEND_METHOD(xiddomdocument, __construct)
{
	zval *id;
	dom_object *intern;
	zend_class_entry **pce;
	zend_class_entry *ce = xiddomdocument_class_entry;
	
	char *encoding, *version = NULL;
	int encoding_len = 0, version_len = 0, refcount;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O|ss", &id, xiddomdocument_class_entry,
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

	// Initialize 'properties' HashTable, which we use to store the pointer to the associated C++
	// XID_DOMDocument object
	php_libxml_node_object *xml_object = (php_libxml_node_object *) intern;
	ALLOC_HASHTABLE(xml_object->properties);
	if (zend_hash_init(xml_object->properties, 50, NULL, (dtor_func_t) propDestructor, 0) == FAILURE) {
		FREE_HASHTABLE(xml_object->properties);
		return;
	}
	
}

xmlDocPtr xid_domdocument_to_libxml_domdocument(XID_DOMDocument *xiddoc)
{
	DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(XMLString::transcode("LS"));
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
		zend_throw_exception(xy_dom_exception_ce, XMLString::transcode(e.getMessage()), 0 TSRMLS_CC);
	}
	catch( const DOMException& e ) {
		zend_throw_exception(xy_xml_exception_ce, XMLString::transcode(e.msg), 0 TSRMLS_CC);
	}
	catch (...) {
		std::cout << "Unexpected Exception" << std::endl;
	}
	
	char* theXMLString_Encoded = (char*) ((MemBufFormatTarget*)myFormatTarget)->getRawBuffer();
	int xmlLen = (int) ((MemBufFormatTarget*)myFormatTarget)->getLen();
	
	char *xmlString = (char *) emalloc(sizeof(char)*xmlLen+1);
	strncpy (xmlString, theXMLString_Encoded, xmlLen);
	xmlString[xmlLen] = '\0';
	
	theOutput->release();
	theSerializer->release();
	
	
	xmlDocPtr newdoc;
	xmlParserCtxtPtr ctxt = NULL;
	
	ctxt = xmlCreateDocParserCtxt((xmlChar *)xmlString);
	if (ctxt == NULL) {
		newdoc = NULL;
	}
	xmlParseDocument(ctxt);
	if (ctxt->wellFormed) {
		newdoc = ctxt->myDoc;
	}
	
	efree(xmlString);
	return newdoc;
}

XID_DOMDocument * libxml_domdocument_to_xid_domdocument(php_libxml_node_object *libxml_doc)
{
	xmlChar *mem = NULL;
	int size = 0;
	DOMDocument *xiddoc;
	dom_doc_propsptr doc_props;
	int format = 0;
	xmlDocPtr docp;
	
	docp = (xmlDocPtr) libxml_doc->document->ptr;
	doc_props = dom_get_doc_props(libxml_doc);
	format = doc_props->formatoutput;
	xmlDocDumpFormatMemory(docp, &mem, &size, format);
	
	const char *xmlString = (const char *)mem;
	
	static const XMLCh gLS[] = { chLatin_L, chLatin_S, chNull };
	try {
		DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(gLS);
		DOMLSParser *theParser = ((DOMImplementationLS*)impl)->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
		DOMErrorHandler * handler = new xydiffPHPParseHandler();
		XMLCh *myWideString = XMLString::transcode(xmlString);
		MemBufInputSource *memIS = new MemBufInputSource((const XMLByte *)xmlString, strlen(xmlString), "test", false);
		Wrapper4InputSource *wrapper = new Wrapper4InputSource(memIS, false);		
		theParser->getDomConfig()->setParameter(XMLUni::fgDOMErrorHandler, handler);
		xiddoc = theParser->parse((DOMLSInput *) wrapper);
	} catch (const XMLException& e) {
		zend_throw_exception(xydiff_exception_ce, XMLString::transcode(e.getMessage()), 0 TSRMLS_CC);
	} catch (const DOMException& toCatch) {
		char* message = XMLString::transcode(toCatch.msg);
		zend_throw_exception(xydiff_exception_ce, XMLString::transcode(toCatch.msg), 0 TSRMLS_CC);
	} catch (...) {
		zend_throw_exception(xydiff_exception_ce, "Unexpected exception during XML parsing", 0 TSRMLS_CC);
	}
	
	if (size) {
		xmlFree(mem);
	}
	XID_DOMDocument *xiddomdoc = new XID_DOMDocument(xiddoc, NULL, true);
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