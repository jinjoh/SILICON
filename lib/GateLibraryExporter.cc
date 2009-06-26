#include "GateLibraryExporter.h"
#include "logic_model.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include "XMLStr.h"
#include <iostream>

XERCES_CPP_NAMESPACE_USE
using namespace std;

GateLibraryExporter * GateLibraryExporter::instance = NULL;

GateLibraryExporter::GateLibraryExporter() {
}

ret_t GateLibraryExporter::init() {
  
  DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(X("Core"));

  if(impl == NULL) return RET_ERR;

  try {
    
    doc = impl->createDocument(0,                 // root element namespace URI.
			       X("root"),         // root element name
			       0);                // document type object (DTD).

    doc->setEncoding(X("ISO-8859-1")); 
    doc->setVersion(X("1.0"));

    rootElem = doc->getDocumentElement();
    

  }
  catch (...) {
    return RET_ERR;
  }
  
  return RET_OK;
}

GateLibraryExporter::~GateLibraryExporter() {
  doc->release();
}

GateLibraryExporter * GateLibraryExporter::get_instance() {
  if(instance == NULL) {
    instance = new GateLibraryExporter();
    if(RET_IS_NOT_OK(instance->init())) {
      delete instance;
      instance = NULL;
    }
  }
  return instance;
}

/*

struct lmodel_gate_template_port {
  unsigned int id;
  char * port_name;
  LM_PORT_TYPE port_type;
  unsigned int relative_x_coord;
  unsigned int relative_y_coord;
  unsigned int diameter;
  color_t color;
  lmodel_gate_template_port_t * next;
};

struct lmodel_gate_template {
  unsigned int id; // a value of zero indicates that this is undefined
  unsigned int master_image_min_x;
  unsigned int master_image_min_y;
  unsigned int master_image_max_x;
  unsigned int master_image_max_y;
  char * short_name;
  char * description;
  color_t fill_color;
  color_t frame_color;
  unsigned int reference_counter;
  lmodel_gate_template_port_t * ports;
};


*/

ret_t GateLibraryExporter::add(const lmodel_gate_template_t * tmpl) {

  DOMElement * gateElem = doc->createElement(X("Gate"));
  rootElem->appendChild(gateElem);

    
  gateElem->setAttribute(X("gateID"), X("1200"));
  gateElem->setAttribute(X("gateType"), X("Nor"));

  
  return RET_OK;
}

ret_t GateLibraryExporter::save_as(std::string filename) {

  ret_t ret = RET_OK;

  DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(X("LS"));
  DOMWriter* theSerializer = ((DOMImplementationLS*)impl)->createDOMWriter();
  
  if (theSerializer->canSetFeature(XMLUni::fgDOMXMLDeclaration,true) )
    theSerializer->setFeature(XMLUni::fgDOMXMLDeclaration, true);

  if(theSerializer->canSetFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true))
    theSerializer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true);

  XMLFormatTarget *myFormTarget = new LocalFileFormatTarget(filename.c_str()); 

   
  try {
    theSerializer->writeNode(myFormTarget, *doc);
  }
  catch (const XMLException& toCatch) {
    char* message = XMLString::transcode(toCatch.getMessage());
    cout << "Exception message is: \n"
	 << message << "\n";
    XMLString::release(&message);
    ret = RET_ERR;
  }
  catch (const DOMException& toCatch) {
    char* message = XMLString::transcode(toCatch.msg);
    cout << "Exception message is: \n"
	 << message << "\n";
    XMLString::release(&message);
    ret = RET_ERR;
  }
  catch (...) {
    cout << "Unexpected Exception \n" ;
    ret = RET_ERR;
  }

  theSerializer->release();
  return ret;
}

