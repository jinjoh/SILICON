#include "LogicExporter.h"
#include "logic_model.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include "XMLStr.h"
#include <iostream>


#define DEBUG 1

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
   
  }
  catch (...) {
    debug(TM, "Excpetion occurred.");
    return RET_ERR;
  }
  
  return RET_OK;
}

LogicExporter::~LogicExporter() {

  if(doc != NULL) doc->release();
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

  ret_t ret;
  if(g == NULL) return RET_INV_PTR;

  if(doc == NULL) init();

  DOMElement * gateElem = doc->createElement(X("gate"));
  gatesElem->appendChild(gateElem);
    
  gateElem->setAttribute(X("id"), X(g->id));
  gateElem->setAttribute(X("name"), X(g->name));
  gateElem->setAttribute(X("min-x"), X(g->min_x));
  gateElem->setAttribute(X("min-y"), X(g->min_y));
  gateElem->setAttribute(X("max-x"), X(g->max_x));
  gateElem->setAttribute(X("max-y"), X(g->max_y));

  switch(g->template_orientation) {
  case LM_TEMPLATE_ORIENTATION_UNDEFINED:
    gateElem->setAttribute(X("orientation"), X("undefined"));
    break;
  case LM_TEMPLATE_ORIENTATION_NORMAL:
    gateElem->setAttribute(X("orientation"), X("normal"));
    break;
  case LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN:
    gateElem->setAttribute(X("orientation"), X("flipped-up-down"));
    break;
  case LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT:
    gateElem->setAttribute(X("orientation"), X("flipped-left-right"));
    break;
  case LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH:
    gateElem->setAttribute(X("orientation"), X("flipped-both"));
    break;
  }

  if(g->gate_template != NULL)
    gateElem->setAttribute(X("type-id"), X(g->gate_template->id));


  

  lmodel_gate_port_t * port_ptr = g->ports;

  while(port_ptr != NULL) {


    // This is a hack, because 'nets' in degate don't have net-ids, yet.
    lmodel_connection_t * conn_ptr = port_ptr->connections;
    lmodel_connection_t * highest_ptr = conn_ptr;
    while(conn_ptr != NULL) {
      if(conn_ptr > highest_ptr) highest_ptr = conn_ptr;
      conn_ptr = conn_ptr->next;
    }

    
    unsigned long net_id = (unsigned long)highest_ptr; // XXX this is a hack

    if(RET_IS_NOT_OK(ret = add_net(net_id))) return ret;
    if(RET_IS_NOT_OK(ret = add_connection(g->id, port_ptr->port_id, net_id))) return ret;

    port_ptr = port_ptr->next;
  }

  return RET_OK;
}


ret_t LogicExporter::add_net(unsigned long net_id) {

  if(nets.find(net_id) == nets.end()) {
    debug(TM, "Create a new net");
    DOMElement * netElem = doc->createElement(X("net"));
    netsElem->appendChild(netElem);

    netElem->setAttribute(X("id"), X(net_id));

    nets[net_id] = netElem;
  }
  else {
    debug(TM, "Net %ld exists aleady", net_id);
  }
  return RET_OK;

}


ret_t LogicExporter::add_connection(unsigned long gate_id, unsigned long port_id, unsigned long net_id) {

  DOMElement * insert_where = nets[net_id];
  assert(insert_where != NULL);
  if(insert_where == NULL) {
    debug(TM, "Can't add a connection to netlist.");
    return RET_INV_PTR;
  }
  else {
    DOMElement * connElem = doc->createElement(X("connection"));
    assert(connElem != NULL);
    if(connElem != NULL) {
      insert_where->appendChild(connElem);

      connElem->setAttribute(X("from-gate"), X(gate_id));
      connElem->setAttribute(X("from-port"), X(port_id));
    }
    else return RET_INV_PTR;
  }

  return RET_OK;
}


ret_t LogicExporter::save_as(std::string filename) {

  ret_t ret = RET_OK;

  if(doc == NULL) init();

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

  doc->release();
  doc = NULL;
  return ret;
}

