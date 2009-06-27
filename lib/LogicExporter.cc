#include "LogicExporter.h"
#include "logic_model.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include "XMLStr.h"
#include <iostream>

XERCES_CPP_NAMESPACE_USE
using namespace std;

LogicExporter * LogicExporter::instance = NULL;

LogicExporter::LogicExporter() {
}

ret_t LogicExporter::init() {
  
  DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(X("Core"));

  if(impl == NULL) return RET_ERR;

  try {
    
    doc = impl->createDocument(0,                 // root element namespace URI.
			       X("logic-model"),  // root element name
			       0);                // document type object (DTD).

    doc->setEncoding(X("ISO-8859-1"));
    doc->setVersion(X("1.0"));
    doc->setStandalone(true);

    gatesElem = doc->createElement(X("gates"));
    doc->getDocumentElement()->appendChild(gatesElem);

    netsElem = doc->createElement(X("nets"));
    doc->getDocumentElement()->appendChild(netsElem);

    connsElem = doc->createElement(X("connections"));
    doc->getDocumentElement()->appendChild(connsElem);
   
  }
  catch (...) {
    return RET_ERR;
  }
  
  return RET_OK;
}

LogicExporter::~LogicExporter() {
  doc->release();
}

LogicExporter * LogicExporter::get_instance() {
  if(instance == NULL) {
    instance = new LogicExporter();
    if(RET_IS_NOT_OK(instance->init())) {
      delete instance;
      instance = NULL;
    }
  }
  return instance;
}


/*
struct lmodel_gate {
  unsigned int min_x, min_y, max_x, max_y;
  unsigned int id;  // object id
  lmodel_gate_template_t * gate_template;
  LM_TEMPLATE_ORIENTATION template_orientation;
  int is_selected;

  lmodel_gate_port_t * ports;
  char * name;
};
*/
ret_t LogicExporter::add(const lmodel_gate_t * g) {


  if(g == NULL) return RET_INV_PTR;

  DOMElement * gateElem = doc->createElement(X("gate"));
  gatesElem->appendChild(gateElem);
    
  gateElem->setAttribute(X("id"), X(g->id));
  gateElem->setAttribute(X("name"), X(g->name));

  if(g->gate_template != NULL)
    gateElem->setAttribute(X("type-id"), X(g->gate_template->id));


  lmodel_gate_port_t * ptr = g->ports;
  while(ptr != NULL) {
    
    add_net((unsigned long)ptr);
    add_connection(g->id, ptr->port_id, (unsigned long)ptr);

    ptr = ptr->next;
  }

  return RET_OK;
}

ret_t LogicExporter::add_connection(unsigned long gate_id, unsigned long port_id, unsigned long net_id) {

  DOMElement * connElem = doc->createElement(X("connection"));
  connsElem->appendChild(connElem);

  connElem->setAttribute(X("from-gate"), X(gate_id));
  connElem->setAttribute(X("from-port"), X(port_id));
  connElem->setAttribute(X("net-id"), X(net_id));

  return RET_OK;
}

ret_t LogicExporter::add_net(unsigned long net_id) {

  if(nets.find(net_id) == nets.end()) {
    DOMElement * netElem = doc->createElement(X("net"));
    netsElem->appendChild(netElem);

    netElem->setAttribute(X("id"), X(net_id));
    nets.insert(net_id);
  }
  return RET_OK;
}

ret_t LogicExporter::save_as(std::string filename) {

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
    myFormTarget->flush();
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
  //delete theSerializer;
  delete myFormTarget;
  return ret;
}

