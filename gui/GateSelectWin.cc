#include "GateSelectWin.h"

#include <assert.h>
#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include <stdlib.h>

#define TM "GateSelectWin.cc"

GateSelectWin::GateSelectWin(Gtk::Window *parent, logic_model_t * const lmodel) {

  assert(lmodel);
  assert(parent);
  this->lmodel = lmodel;
  this->parent = parent;
  result = NULL;

  char file[PATH_MAX];
  snprintf(file, PATH_MAX, "%s/glade/gate_select.glade", getenv("DEGATE_HOME"));

  //Load the Glade file and instiate its widgets:
  Glib::RefPtr<Gnome::Glade::Xml> refXml;
#ifdef GLIBMM_EXCEPTIONS_ENABLED
  try {
    refXml = Gnome::Glade::Xml::create(file);
  }
  catch(const Gnome::Glade::XmlError& ex) {
    std::cerr << ex.what() << std::endl;
    return;
  }
#else
  std::auto_ptr<Gnome::Glade::XmlError> error;
  refXml = Gnome::Glade::Xml::create(file, "", "", error);
  if(error.get()) {
    std::cerr << error->what() << std::endl;
    return;
  }
#endif

  //Get the Glade-instantiated Dialog:
  refXml->get_widget("gate_select_dialog", pDialog);
  if(pDialog) {
    //Get the Glade-instantiated Button, and connect a signal handler:
    Gtk::Button* pButton = NULL;
    
    // connect signals
    refXml->get_widget("cancel_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateSelectWin::on_cancel_button_clicked));
    
    refXml->get_widget("ok_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateSelectWin::on_ok_button_clicked) );
  

    refListStore = Gtk::ListStore::create(m_Columns);
  
    refXml->get_widget("treeview", pTreeView);
    if(pTreeView) {
      pTreeView->set_model(refListStore);
      pTreeView->append_column("ID", m_Columns.m_col_id);
      pTreeView->append_column("Short Name", m_Columns.m_col_short_name);
      pTreeView->append_column("Description", m_Columns.m_col_description);
    }
    
    gate_set_t * ptr = lmodel->gate_set;
    while(ptr != NULL) {
      if(ptr->gate) {
	Gtk::TreeModel::Row row = *(refListStore->append()); 
	
	row[m_Columns.m_col_id] = ptr->gate->id;
	if(ptr->gate->short_name) row[m_Columns.m_col_short_name] = ptr->gate->short_name;
	if(ptr->gate->description) row[m_Columns.m_col_description] = ptr->gate->description;
      }
      
      ptr = ptr->next;
    }
  }
}

GateSelectWin::~GateSelectWin() {
  delete pDialog;
}


gate_template_t * GateSelectWin::run() {
  pDialog->run();
  return result;
}

void GateSelectWin::on_ok_button_clicked() {
  result = NULL;
  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =  pTreeView->get_selection();
  if(refTreeSelection) {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(iter && *iter) {
      Gtk::TreeModel::Row row = *iter; 
      int obj_id = row[m_Columns.m_col_id];
      result = lmodel_get_gate_template_by_id(lmodel, obj_id);
      pDialog->hide();
    }
  }
}

void GateSelectWin::on_cancel_button_clicked() {
  result = NULL;
  pDialog->hide();
}

