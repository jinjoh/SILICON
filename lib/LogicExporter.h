#ifndef __LOGICEXPORTER_H__
#define __LOGICEXPORTER_H__

#include "logic_model.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <string>
#include <map>

class LogicExporter {

 private:

  static LogicExporter * instance;

  XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc;

  XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* gatesElem;
  XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* netsElem;

  std::map<unsigned long /*net_id*/, XERCES_CPP_NAMESPACE_QUALIFIER DOMElement * /*net_node*/> nets;

  ret_t init();
  LogicExporter();


  ret_t add_connection(unsigned long gate_id, unsigned long port_id, unsigned long net_id);
  ret_t add_net(unsigned long net_id);
  
 public:
  ~LogicExporter();

  static LogicExporter * get_instance();
  ret_t add(const lmodel_gate_t * templ);
  ret_t save_as(std::string filename);
};


#endif
