#ifndef __GATELIBRARYEXPORTER_H__
#define __GATELIBRARYEXPORTER_H__

#include "logic_model.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <string>

class GateLibraryExporter {

 private:

  static GateLibraryExporter * instance;

  XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc;
  XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* rootElem;


  ret_t init();
  GateLibraryExporter();
  
 public:
  ~GateLibraryExporter();

  static GateLibraryExporter * get_instance();
  ret_t add(const lmodel_gate_template_t * templ);
  ret_t save_as(std::string filename);
};


#endif
