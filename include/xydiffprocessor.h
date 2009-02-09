

//
//
//XERCES_CPP_NAMESPACE_USE
//
//class xydeltaParseHandler : public DOMErrorHandler {
//public:
//	void warning(const SAXParseException& e);
//	void error(const SAXParseException& e);
//	void fatalError(const SAXParseException& e);
//	void resetErrors() {};
//	bool handleError(const DOMError& domError);
//} ;
//
//
//bool xydeltaParseHandler::handleError(const DOMError& domError)
//{
//	DOMLocator* locator = domError.getLocation();
//	std::cerr << "\n(GF) Error at (file " << XyLatinStr(locator->getURI()).localForm()
//	<< ", line " << locator->getLineNumber()
//	<< ", char " << locator->getColumnNumber()
//	<< "): " << XyLatinStr(domError.getMessage()).localForm() << std::endl;
//	throw VersionManagerException("xydeltaParseHandler", "error", "-");
//}
//
//void xydeltaParseHandler::error(const SAXParseException& e) {
//	std::cerr << "\n(GF) Error at (file " << XyLatinStr(e.getSystemId()).localForm()
//	<< ", line " << e.getLineNumber()
//	<< ", char " << e.getColumnNumber()
//	<< "): " << XyLatinStr(e.getMessage()).localForm() << std::endl;
//	throw VersionManagerException("xydeltaParseHandler", "error", "-");
//}
//void xydeltaParseHandler::fatalError(const SAXParseException& e) {
//	std::cerr << "\n(GF) Fatal Error at (file " << XyLatinStr(e.getSystemId()).localForm()
//	<< ", line " << e.getLineNumber()
//	<< ", char " << e.getColumnNumber()
//	<< "): " << XyLatinStr(e.getMessage()).localForm() << std::endl;
//	throw VersionManagerException("xydeltaParseHandler", "fatal error", "-");
//}
//void xydeltaParseHandler::warning(const SAXParseException& e) {
//	std::cerr << "\n(GF) Warning at (file " << XyLatinStr(e.getSystemId()).localForm()
//	<< ", line " << e.getLineNumber()
//	<< ", char " << e.getColumnNumber()
//	<< "): " << XyLatinStr(e.getMessage()).localForm() << std::endl;
//}
//
//
