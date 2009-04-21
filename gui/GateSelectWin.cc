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

#include "GateSelectWin.h"

#include <assert.h>
#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include <stdlib.h>

GateSelectWin::GateSelectWin(Gtk::Window *parent, logic_model_t * const lmodel) {

  assert(lmodel);
  assert(parent);
  this->lmodel = lmodel;
  this->parent = parent;

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
    
    refXml->get_widget("ok_button", pOkButton);
    if(pOkButton) {
      pOkButton->set_sensitive(false);
      pOkButton->signal_clicked().connect(sigc::mem_fun(*this, &GateSelectWin::on_ok_button_clicked) );         
    }

    refListStore = Gtk::ListStore::create(m_Columns);
  
    refXml->get_widget("treeview", pTreeView);
    if(pTreeView) {
      pTreeView->set_model(refListStore);
      //pTreeView->append_column("ID", m_Columns.m_col_id);
      pTreeView->append_column("Short Name", m_Columns.m_col_short_name);
      pTreeView->append_column("#", m_Columns.m_col_refcount);
      pTreeView->append_column("Width", m_Columns.m_col_width);
      pTreeView->append_column("Height", m_Columns.m_col_height);
      pTreeView->append_column("Description", m_Columns.m_col_description);

      Gtk::TreeView::Column * pColumn;

      //pColumn = pTreeView->get_column(0);
      //if(pColumn) pColumn->set_sort_column(m_Columns.m_col_id);
      
      pColumn = pTreeView->get_column(0);
      if(pColumn) pColumn->set_sort_column(m_Columns.m_col_refcount);
      
      pColumn = pTreeView->get_column(1);
      if(pColumn) pColumn->set_sort_column(m_Columns.m_col_width);
      
      pColumn = pTreeView->get_column(2);
      if(pColumn) pColumn->set_sort_column(m_Columns.m_col_height);
      
      pColumn = pTreeView->get_column(3);
      if(pColumn) pColumn->set_sort_column(m_Columns.m_col_short_name);
      
      pColumn = pTreeView->get_column(4);
      if(pColumn) pColumn->set_sort_column(m_Columns.m_col_description);

      Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = pTreeView->get_selection();
      refTreeSelection->signal_changed().connect(sigc::mem_fun(*this, &GateSelectWin::on_selection_changed));

      refListStore->set_sort_column_id(m_Columns.m_col_short_name, Gtk::SORT_ASCENDING);
    }
    
    lmodel_gate_template_set_t * ptr = lmodel->gate_template_set;
    while(ptr != NULL) {
      if(ptr->gate) {
	Gtk::TreeModel::Row row = *(refListStore->append()); 
	
	row[m_Columns.m_col_id] = ptr->gate->id;
	row[m_Columns.m_col_refcount] = ptr->gate->reference_counter;
	row[m_Columns.m_col_width] = ptr->gate->master_image_max_x - ptr->gate->master_image_min_x;
	row[m_Columns.m_col_height] = ptr->gate->master_image_max_y - ptr->gate->master_image_min_y;

	if(ptr->gate->short_name) row[m_Columns.m_col_short_name] = ptr->gate->short_name;
	if(ptr->gate->description) row[m_Columns.m_col_description] = ptr->gate->description;
      }
      
      ptr = ptr->next;
    }

  }
}


void GateSelectWin::on_selection_changed() {
  debug(TM, "sth. selected");
  pOkButton->set_sensitive(true);
}

GateSelectWin::~GateSelectWin() {
  delete pDialog;
}


lmodel_gate_template_t * GateSelectWin::get_single() {
  lmodel_gate_template_t * result = NULL;
  pDialog->run();

  if(ok_clicked == false) return NULL;

  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =  pTreeView->get_selection();
  Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
  if(iter && *iter) {
    Gtk::TreeModel::Row row = *iter; 
    int obj_id = row[m_Columns.m_col_id];
    result = lmodel_get_gate_template_by_id(lmodel, obj_id);
    debug(TM, "selected single template: [%s]", result->short_name);

  }

  return result;
}

lmodel_gate_template_set_t * GateSelectWin::get_multiple() {
  lmodel_gate_template_set_t * result = NULL;

  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =  pTreeView->get_selection();

  refTreeSelection->set_mode(Gtk::SELECTION_MULTIPLE);
  pDialog->run();

  if(ok_clicked == false) return NULL;

  std::vector<Gtk::TreeModel::Path> pathlist = refTreeSelection->get_selected_rows();

  for(std::vector<Gtk::TreeModel::Path>::iterator iter = pathlist.begin(); iter != pathlist.end(); ++iter) {
    Gtk::TreeModel::Row row = *(refTreeSelection->get_model()->get_iter (*iter));

    int obj_id = row[m_Columns.m_col_id];
    lmodel_gate_template_t * tmpl = lmodel_get_gate_template_by_id(lmodel, obj_id);
    debug(TM, "selected template: [%s]", tmpl->short_name);
    
    if(result == NULL) result  = lmodel_create_gate_template_set(tmpl);
    else lmodel_add_gate_template_to_gate_template_set(result, tmpl, 0);
  }
  
  return result;
}

void GateSelectWin::on_ok_button_clicked() {
  ok_clicked = true;
  pDialog->hide();
}

void GateSelectWin::on_cancel_button_clicked() {
  ok_clicked = false;
  pDialog->hide();
}

