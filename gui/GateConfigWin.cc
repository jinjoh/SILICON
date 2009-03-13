#include "GateConfigWin.h"
#include "lib/grid.h"

#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>

#include <stdlib.h>

#define TM "GateConfigWin.cc"

GateConfigWin::GateConfigWin(Gtk::Window *parent, gate_template_t * const gate_template) {
  char file[PATH_MAX];
  snprintf(file, PATH_MAX, "%s/glade/gate_create.glade", getenv("DEGATE_HOME"));
  port_counter = 0;
  
  this->gate_template = gate_template;

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
      return ;
    }
#endif

    //Get the Glade-instantiated Dialog:
    refXml->get_widget("gate_create_dialog", pDialog);
    if(pDialog) {
      //Get the Glade-instantiated Button, and connect a signal handler:
      Gtk::Button* pButton = NULL;

      // connect signals
      refXml->get_widget("cancel_button", pButton);
      if(pButton)
 	pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateConfigWin::on_cancel_button_clicked));

      refXml->get_widget("ok_button", pButton);
      if(pButton)
	pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateConfigWin::on_ok_button_clicked) );

      refXml->get_widget("inport_add_button", pButton);
      if(pButton)
	pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateConfigWin::on_inport_add_button_clicked) );

      refXml->get_widget("inport_remove_button", pButton);
      if(pButton)
	pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateConfigWin::on_inport_remove_button_clicked) );

      refXml->get_widget("outport_add_button", pButton);
      if(pButton)
	pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateConfigWin::on_outport_add_button_clicked) );

      refXml->get_widget("outport_remove_button", pButton);
      if(pButton)
	pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateConfigWin::on_outport_remove_button_clicked) );

      refListStore_out_ports = Gtk::ListStore::create(m_Columns);
      refListStore_in_ports = Gtk::ListStore::create(m_Columns);

      refXml->get_widget("treeview_outports", pTreeView_out_ports);
      if(pTreeView_out_ports) {
	pTreeView_out_ports->set_model(refListStore_out_ports);
	pTreeView_out_ports->append_column("Port ID", m_Columns.m_col_id);
	pTreeView_out_ports->append_column_editable("Port Name", m_Columns.m_col_text);
      }

      refXml->get_widget("treeview_inports", pTreeView_in_ports);
      if(pTreeView_in_ports) {
	pTreeView_in_ports->set_model(refListStore_in_ports);
	pTreeView_in_ports->append_column("Port ID", m_Columns.m_col_id);
	pTreeView_in_ports->append_column_editable("Port Name", m_Columns.m_col_text);
      }

      gate_template_port_t * ptr = gate_template->ports;
      while(ptr) {
	Gtk::TreeModel::Row row;

	debug(TM, "PORT NAME: [%s]", ptr->port_name);
	if(ptr->port_type == LM_PT_OUT)
	  row = *(refListStore_out_ports->append()); 
	else
	  row = *(refListStore_in_ports->append()); 

	row[m_Columns.m_col_text] = ptr->port_name;
	row[m_Columns.m_col_id] = ptr->id;
	if(ptr->id >= port_counter) port_counter = ptr->id + 1;

	ptr = ptr->next;
      }
      
      refXml->get_widget("entry_short_name", entry_short_name);
      refXml->get_widget("entry_description", entry_description);

      if(entry_short_name && gate_template->short_name) 
	entry_short_name->set_text(gate_template->short_name);
      if(entry_description && gate_template->description) 
	entry_description->set_text(gate_template->description);

    }
}

GateConfigWin::~GateConfigWin() {
  delete pDialog;
}


bool GateConfigWin::run() {
  pDialog->run();
  std::cout << "after run()" << std::endl;
  return result;
}

void GateConfigWin::on_ok_button_clicked() {
  Glib::ustring str;
  unsigned int id;
  // get text content and set it to the gate template
  lmodel_gate_template_set_text(gate_template, 
				entry_short_name->get_text().c_str(),
				entry_description->get_text().c_str());


  // get ports
  typedef Gtk::TreeModel::Children type_children;

  type_children children = refListStore_in_ports->children();
  for(type_children::iterator iter = children.begin(); iter != children.end(); ++iter) {
    Gtk::TreeModel::Row row = *iter;
    str = row[m_Columns.m_col_text];
    id = row[m_Columns.m_col_id];
    lmodel_gate_template_set_port(gate_template, id, str.c_str(), LM_PT_IN);
  }

  children = refListStore_out_ports->children();
  for(type_children::iterator iter = children.begin(); iter != children.end(); ++iter) {
    Gtk::TreeModel::Row row = *iter;
    str = row[m_Columns.m_col_text];
    id = row[m_Columns.m_col_id];
    lmodel_gate_template_set_port(gate_template, id, str.c_str(), LM_PT_OUT);
  }

  pDialog->hide();
  result = true;
}

void GateConfigWin::on_cancel_button_clicked() {
  pDialog->hide();
  result = false;
}

void GateConfigWin::on_inport_add_button_clicked() {
  Gtk::TreeModel::Row row = *(refListStore_in_ports->append()); 
  row[m_Columns.m_col_text] = "click to edit";
  row[m_Columns.m_col_id] = port_counter++;
}

void GateConfigWin::on_inport_remove_button_clicked() {
  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =  pTreeView_in_ports->get_selection();
  if(refTreeSelection) {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(*iter) refListStore_in_ports->erase(iter);
  }
}

void GateConfigWin::on_outport_add_button_clicked() {
  Gtk::TreeModel::Row row = *(refListStore_out_ports->append()); 
  row[m_Columns.m_col_text] = "click to edit";
  row[m_Columns.m_col_id] = port_counter++;
}

void GateConfigWin::on_outport_remove_button_clicked() {
  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =  pTreeView_out_ports->get_selection();
  if(refTreeSelection) {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(*iter) refListStore_out_ports->erase(iter);
  }
}

