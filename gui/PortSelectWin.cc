/*                                                                              
                                                                                
This file is part of the IC reverse engineering tool degate.                    
                                                                                
Copyright 2008, 2009 by Martin Schobert                                         
                                                                                
Degate is free software: you can redistribute it and/or modify                  
it under the terms of the GNU General Public License as published by            
the Free Software Foundation, either version 3 of the License, or               
any later version.                                                              
                                                                                
Degate is distributed in the hope that it will be useful,                       
but WITHOUT ANY WARRANTY; without even the implied warranty of                  
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                   
GNU General Public License for more details.                                    
                                                                                
You should have received a copy of the GNU General Public License               
along with degate. If not, see <http://www.gnu.org/licenses/>.                  
                                                                                
*/

#include "PortSelectWin.h"

#include <assert.h>
#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include <stdlib.h>

PortSelectWin::PortSelectWin(Gtk::Window *parent, lmodel_gate_t * gate) {

  assert(gate->gate_template);
  assert(gate->gate_template->ports);
  assert(parent);
  this->parent = parent;
  this->gate = gate;

  char file[PATH_MAX];
  snprintf(file, PATH_MAX, "%s/glade/port_select.glade", getenv("DEGATE_HOME"));

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
  refXml->get_widget("port_select_dialog", pDialog);
  if(pDialog) {
    //Get the Glade-instantiated Button, and connect a signal handler:
    Gtk::Button* pButton = NULL;
    
    // connect signals
    refXml->get_widget("cancel_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &PortSelectWin::on_cancel_button_clicked));
    
    refXml->get_widget("ok_button", pButton);
    if(pButton) {
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &PortSelectWin::on_ok_button_clicked) );
      pButton->grab_focus();
    }

    refListStore = Gtk::ListStore::create(m_Columns);
  
    refXml->get_widget("treeview", pTreeView);
    if(pTreeView) {
      pTreeView->set_model(refListStore);
      pTreeView->append_column("ID", m_Columns.m_col_id);
      pTreeView->append_column("Port Name", m_Columns.m_col_name);
    }
    

    lmodel_gate_template_port_t * ptr = gate->gate_template->ports;
    while(ptr != NULL) {

      Gtk::TreeModel::Row row = *(refListStore->append()); 
	
      row[m_Columns.m_col_id] = ptr->id;
      if(ptr->port_name) row[m_Columns.m_col_name] = ptr->port_name;
      
      ptr = ptr->next;
    }
  }
}

PortSelectWin::~PortSelectWin() {
  delete pDialog;
}


lmodel_gate_template_port_t * PortSelectWin::run() {
  pDialog->run();
  return template_port;
}

void PortSelectWin::on_ok_button_clicked() {
  template_port = NULL;
  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =  pTreeView->get_selection();
  if(refTreeSelection) {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(iter && *iter) {
      Gtk::TreeModel::Row row = *iter; 
      unsigned int port_id = row[m_Columns.m_col_id];
      assert(gate->gate_template);
      assert(gate->gate_template->ports);
      
      lmodel_gate_template_port_t * ptr = gate->gate_template->ports;
      while(ptr != NULL) {
	if(ptr->id == port_id) {
	  template_port = ptr;
	  break;
	}
	ptr = ptr->next;
      }
      //result = lmodel_get_gate_template_by_id(lmodel, obj_id);
      pDialog->hide();
    }
  }
}

void PortSelectWin::on_cancel_button_clicked() {
  template_port = NULL;
  pDialog->hide();
}

