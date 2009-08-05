#include "ProjectExporter.h"
#include "project.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include "XMLStr.h"
#include <iostream>

XERCES_CPP_NAMESPACE_USE
using namespace std;

ProjectExporter * ProjectExporter::instance = NULL;

ProjectExporter::ProjectExporter() : doc(NULL) {
}

ret_t ProjectExporter::init() {
  
  if(doc != NULL) return RET_OK;

  DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(X("Core"));

  if(impl == NULL) return RET_ERR;

  try {
    
    doc = impl->createDocument(0,                 // root element namespace URI.
			       X("project"),      // root element name
			       0);                // document type object (DTD).

    doc->setEncoding(X("ISO-8859-1"));
    doc->setVersion(X("1.0"));
    doc->setStandalone(true);

    rootElem = doc->getDocumentElement();
   
    gridsElem = doc->createElement(X("grids"));
    rootElem->appendChild(gridsElem);

    layersElem = doc->createElement(X("layers"));
    rootElem->appendChild(layersElem);

    portcolorsElem = doc->createElement(X("port-colors"));
    rootElem->appendChild(portcolorsElem);

  }
  catch (...) {
    return RET_ERR;
  }
  
  return RET_OK;
}

ProjectExporter::~ProjectExporter() {
  if(doc != NULL) doc->release();
}

ProjectExporter * ProjectExporter::get_instance() {
  if(instance == NULL) {
    instance = new ProjectExporter();
  }

  if(RET_IS_NOT_OK(instance->init())) {
    delete instance;
    instance = NULL;
  }

  return instance;
}

/*
struct project {

  int num_layers;

  char * project_dir;
  
  image_t ** bg_images;
  scaling_manager_t * scaling_manager;
  port_color_manager_t * port_color_manager;

  logic_model_t * lmodel;
  
  int current_layer;

  unsigned int pin_diameter;
  unsigned int wire_diameter;
  unsigned int lambda;

  grid_t * grid;

  alignment_marker_set_t * alignment_marker_set;
  char * project_file_version;
};
*/

ret_t ProjectExporter::set_project(const project_t * prj) {

  if(prj == NULL || prj->lmodel == NULL) return RET_INV_PTR;

  rootElem->setAttribute(X("name"), X(prj->project_name));
  rootElem->setAttribute(X("description"), X(prj->project_description));
  rootElem->setAttribute(X("width"), X(prj->width));
  rootElem->setAttribute(X("height"), X(prj->height));

  rootElem->setAttribute(X("pin-diameter"), X(prj->pin_diameter));
  rootElem->setAttribute(X("wire-diameter"), X(prj->wire_diameter));
  rootElem->setAttribute(X("lambda"), X(prj->lambda));
  rootElem->setAttribute(X("degate-version"), X(prj->project_file_version));
  rootElem->setAttribute(X("object-id-counter"), X(prj->lmodel->object_id_counter));

    
  for(int i = 0; i < prj->num_layers; i++) {
    DOMElement * layerElem = doc->createElement(X("layer"));
    layersElem->appendChild(layerElem);

    layerElem->setAttribute(X("position"), X(i));
    
    switch(prj->lmodel->layer_type[i]) {
    case LM_LAYER_TYPE_UNDEF:
      layerElem->setAttribute(X("type"), X("undefined"));
      break;

    case LM_LAYER_TYPE_METAL:
      layerElem->setAttribute(X("type"), X("metal"));
      break;

    case LM_LAYER_TYPE_LOGIC:
      layerElem->setAttribute(X("type"), X("logic"));
      break;

    case LM_LAYER_TYPE_TRANSISTOR:
      layerElem->setAttribute(X("type"), X("transistor"));
      break;
    }

    char bg_mapping_filename[PATH_MAX];
    snprintf(bg_mapping_filename, sizeof(bg_mapping_filename), "bg_layer_%02d.dat", i);
    layerElem->setAttribute(X("image-filename"), X(bg_mapping_filename));

  }


  /*
    <regular-grid orientation="horizontal" enabled="true" offset="0" distance="23.42"/>

  */
  DOMElement * regularGridElem = doc->createElement(X("regular-grid"));
  gridsElem->appendChild(regularGridElem);
  
  regularGridElem->setAttribute(X("orientation"), X("horizontal"));
  regularGridElem->setAttribute(X("enabled"), 
				prj->grid->horizontal_lines_enabled ? X("true") : X("false"));
  regularGridElem->setAttribute(X("offset"), X(prj->grid->offset_x));
  regularGridElem->setAttribute(X("distance"), X(prj->grid->dist_x));


  //
  regularGridElem = doc->createElement(X("regular-grid"));
  gridsElem->appendChild(regularGridElem);

  regularGridElem->setAttribute(X("orientation"), X("vertical"));
  regularGridElem->setAttribute(X("enabled"), 
				prj->grid->vertical_lines_enabled ? X("true") : X("false"));
  regularGridElem->setAttribute(X("offset"), X(prj->grid->offset_y));
  regularGridElem->setAttribute(X("distance"), X(prj->grid->dist_y));

  //
  DOMElement * irregularGridElem = doc->createElement(X("irregular-grid"));
  gridsElem->appendChild(irregularGridElem);
  irregularGridElem->setAttribute(X("orientation"), X("horizontal"));
  irregularGridElem->setAttribute(X("enabled"), 
				prj->grid->uhg_enabled ? X("true") : X("false"));

  DOMElement * offsetsElem = doc->createElement(X("offsets"));
  irregularGridElem->appendChild(offsetsElem);
  for(unsigned int i = 0; i < prj->grid->num_uhg_entries; i++) {
    DOMElement * offsetElem = doc->createElement(X("offset-entry"));
    offsetsElem->appendChild(offsetElem);
    offsetElem->setAttribute(X("offset"), X(prj->grid->uhg_offsets[i]));
  }

  //
  irregularGridElem = doc->createElement(X("irregular-grid"));
  gridsElem->appendChild(irregularGridElem);
  irregularGridElem->setAttribute(X("orientation"), X("vertical"));
  irregularGridElem->setAttribute(X("enabled"), 
				prj->grid->uvg_enabled ? X("true") : X("false"));

  offsetsElem = doc->createElement(X("offsets"));
  irregularGridElem->appendChild(offsetsElem);
  for(unsigned int i = 0; i < prj->grid->num_uvg_entries; i++) {
    DOMElement * offsetElem = doc->createElement(X("offset-entry"));
    offsetsElem->appendChild(offsetElem);
    offsetElem->setAttribute(X("offset"), X(prj->grid->uvg_offsets[i]));
  }


  //

  port_color_list_t * ptr = prj->port_color_manager->port_color_list;
  while(ptr != NULL) {

    DOMElement * pcolorElem = doc->createElement(X("port-color"));
    portcolorsElem->appendChild(pcolorElem);
    
    char color_text[10];
    snprintf(color_text, sizeof(color_text), 
	     "#%02X%02X%02X", 
	     MASK_R(ptr->color), MASK_G(ptr->color), MASK_B(ptr->color));

    pcolorElem->setAttribute(X("color"), X(color_text));
    pcolorElem->setAttribute(X("alpha"), X(MASK_A(ptr->color)));
    pcolorElem->setAttribute(X("port-name"), X(ptr->port_name));

    ptr = ptr->next;
  }


  return RET_OK;
}

ret_t ProjectExporter::save_as(std::string filename) {

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

  doc->release();
  doc = NULL;

  return ret;
}

