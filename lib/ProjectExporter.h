#ifndef __PROJECTEXPORTER_H__
#define __PROJECTEXPORTER_H__

#include "project.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <string>

class ProjectExporter {

 private:

  static ProjectExporter * instance;

  XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc;
  XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* rootElem;
  XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* gridsElem;
  XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* layersElem;
  XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* portcolorsElem;


  ret_t init();
  ProjectExporter();
  
 public:
  ~ProjectExporter();

  static ProjectExporter * get_instance();
  ret_t set_project(const project_t * prj);
  ret_t save_as(std::string filename);
};


#endif
