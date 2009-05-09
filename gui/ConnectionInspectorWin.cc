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

#include "ConnectionInspectorWin.h"
#include "lib/grid.h"

#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <set>

#define MY_GREY "#808080"
#define MY_WHITE "#ffffff"
#define MY_BLUE "#c0c0ff"
#define DEFAULT_WIDTH 90

ConnectionInspectorWin::ConnectionInspectorWin(Gtk::Window *parent, logic_model_t * lmodel) {

  this->lmodel = lmodel;
  this->parent = parent;

  char file[PATH_MAX];
  snprintf(file, PATH_MAX, "%s/glade/connection_inspector.glade", getenv("DEGATE_HOME"));

  assert(lmodel);

  set_opacity(0.5);


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
    refXml->get_widget("connection_inspector_dialog", pDialog);
    assert(pDialog);
    if(pDialog) {

      pDialog->set_opacity(0.5);

      // connect signals
      refXml->get_widget("close_button", pCloseButton);
      if(pCloseButton)
 	pCloseButton->signal_clicked().connect(sigc::mem_fun(*this, &ConnectionInspectorWin::on_close_button_clicked));

      refXml->get_widget("goto_button", pGotoButton);
      if(pGotoButton) {
	pGotoButton->grab_focus();
	pGotoButton->signal_clicked().connect(sigc::mem_fun(*this, &ConnectionInspectorWin::on_goto_button_clicked) );
      }

      refXml->get_widget("back_button", pBackButton);
      if(pBackButton) {
	pBackButton->signal_clicked().connect(sigc::mem_fun(*this, &ConnectionInspectorWin::on_back_button_clicked) );
      }

      refXml->get_widget("current_object_label", current_object_label);
      refXml->get_widget("current_object_type_label", current_object_type_label);

      refListStore = Gtk::ListStore::create(m_Columns);

      refXml->get_widget("treeview", pTreeView);
      if(pTreeView) {
	pTreeView->set_model(refListStore);

	Gtk::CellRendererText * pRenderer = Gtk::manage( new Gtk::CellRendererText()); 
	Gtk::TreeView::Column * pColumn;

	/*
	 * col 0
	 */
	pTreeView->append_column("Previous", *pRenderer);

	pColumn = pTreeView->get_column(0);
	// text attribute is the text to show
	pColumn->add_attribute(*pRenderer, "text", m_Columns.m_col_prev_name);  
	pColumn->add_attribute(*pRenderer, "background", m_Columns.color_); 

	pColumn->set_resizable(true); 
	pColumn->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
	pColumn->set_min_width(DEFAULT_WIDTH);

	if(pColumn) pColumn->set_sort_column(m_Columns.m_col_prev_name);
	//refListStore->set_sort_column(m_Columns.m_col_prev_name, Gtk::SORT_ASCENDING);

	/*
	 * col 1
	 */

	pTreeView->append_column("Current", *pRenderer);
	pColumn = pTreeView->get_column(1);
	// text attribute is the text to show
	pColumn->add_attribute(*pRenderer, "text", m_Columns.m_col_curr_name);
	pColumn->add_attribute(*pRenderer, "background", m_Columns.color_);

	pColumn->set_resizable(true); 
	pColumn->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
	pColumn->set_min_width(DEFAULT_WIDTH);

	//if(pColumn) pColumn->set_sort_column(m_Columns.m_col_curr_name_sort);
	//refListStore->set_sort_column(m_Columns.m_col_curr_name_sort, Gtk::SORT_ASCENDING);

	/*
	 * col 2
	 */

	pTreeView->append_column("Next", *pRenderer);
	pColumn = pTreeView->get_column(2);	
	// text attribute is the text to show
	pColumn->add_attribute(*pRenderer, "text", m_Columns.m_col_next_name);
	pColumn->add_attribute(*pRenderer, "background", m_Columns.color_);

	pColumn->set_resizable(true); 
	pColumn->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
	pColumn->set_min_width(DEFAULT_WIDTH);

	if(pColumn) pColumn->set_sort_column(m_Columns.m_col_next_name);
	//refListStore->set_sort_column(m_Columns.m_col_next_name, Gtk::SORT_ASCENDING);



	refListStore->set_sort_column_id(m_Columns.m_col_curr_name_sort, Gtk::SORT_ASCENDING);

	// signal
	Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = pTreeView->get_selection();
	refTreeSelection->signal_changed().connect(sigc::mem_fun(*this, &ConnectionInspectorWin::on_selection_changed));

      }

    }

    //pDialog->set_transient_for(*parent);
    disable_inspection();
}

ConnectionInspectorWin::~ConnectionInspectorWin() {
  delete pDialog;
}

void ConnectionInspectorWin::show() {
  pDialog->show();
}


void ConnectionInspectorWin::on_selection_changed() {

  pGotoButton->set_sensitive(true);
}


void ConnectionInspectorWin::show_connections(LM_OBJECT_TYPE src_object_type,
					      object_ptr_t * src_curr_obj) {

  
  char str[100];
  lmodel_wire_t * wire = NULL;
  lmodel_via_t * via = NULL;
  lmodel_gate_port_t * gate_port = NULL;
  lmodel_gate_template_port_t * tmpl_port = NULL;
  Gtk::TreeModel::Row row;
  Glib::ustring last_port_name;

  lmodel_connection_t * conn = NULL; 

  if((conn = lmodel_get_connections_from_object(src_object_type, src_curr_obj)) == NULL)
    return;

  while(conn != NULL) {

    if(conn->obj_ptr != src_curr_obj) {

      row = *(refListStore->append()); 

      row[m_Columns.m_col_curr_object_type] = src_object_type;
      row[m_Columns.m_col_curr_object_ptr] = src_curr_obj;
      if(RET_IS_OK(lmodel_get_printable_string_for_obj(src_object_type, src_curr_obj, 
						       str, sizeof(str)))) {
	row[m_Columns.m_col_curr_name] = str;
	row[m_Columns.m_col_curr_name_sort] = str;

	if(src_object_type == LM_TYPE_GATE_PORT) {
	  gate_port = (lmodel_gate_port_t *)src_curr_obj;
	  
	  if(gate_port != NULL && gate_port->tmpl_port != NULL) {
	    //debug(TM, "change name");
	    if(gate_port->tmpl_port->port_type == LM_PT_IN)
	      row[m_Columns.m_col_curr_name_sort] = Glib::ustring("i") + row[m_Columns.m_col_curr_name_sort];
	    else
	      row[m_Columns.m_col_curr_name_sort] = Glib::ustring("o") + row[m_Columns.m_col_curr_name_sort];
	  }
	}
      }

      if(last_port_name != row[m_Columns.m_col_curr_name]) {
	row[m_Columns.color_] = MY_BLUE;
      }
      else {
	row[m_Columns.color_] = MY_WHITE;
      }
      last_port_name = row[m_Columns.m_col_curr_name];
      
      switch(conn->object_type) {
      case LM_TYPE_WIRE:
	wire = (lmodel_wire_t *)conn->obj_ptr;
      
	if(RET_IS_OK(lmodel_get_printable_string_for_obj(LM_TYPE_WIRE, wire, str, sizeof(str)))) {
	  row[m_Columns.m_col_next_name] = str;
	  row[m_Columns.m_col_prev_name] = str;
	}
	
	row[m_Columns.m_col_next_object_type] = LM_TYPE_WIRE;
	row[m_Columns.m_col_next_object_ptr] = (object_ptr_t *)wire;

	row[m_Columns.m_col_prev_object_type] = LM_TYPE_WIRE;
	row[m_Columns.m_col_prev_object_ptr] = (object_ptr_t *)wire;

	//row[m_Columns.color_] = conn->obj_ptr != src_curr_obj ? "black" : MY_GREY;
	break;

      case LM_TYPE_VIA:
	via = (lmodel_via_t *)conn->obj_ptr;
	
	if(RET_IS_OK(lmodel_get_printable_string_for_obj(LM_TYPE_VIA, via, str, sizeof(str)))) {
	  row[m_Columns.m_col_next_name] = str;
	  row[m_Columns.m_col_prev_name] = str;
	}
	
	row[m_Columns.m_col_next_object_type] = LM_TYPE_VIA;
	row[m_Columns.m_col_next_object_ptr] = (object_ptr_t *)via;
	
	row[m_Columns.m_col_prev_object_type] = LM_TYPE_VIA;
	row[m_Columns.m_col_prev_object_ptr] = (object_ptr_t *)via;
	
	//row[m_Columns.color_] = conn->obj_ptr != src_curr_obj ? "black" : MY_GREY;	
	break;
	
      case LM_TYPE_GATE_PORT:
      
	gate_port = (lmodel_gate_port_t *)conn->obj_ptr;
	tmpl_port = gate_port->tmpl_port;
	
	if(RET_IS_OK(lmodel_get_printable_string_for_obj(LM_TYPE_GATE_PORT, 
							 gate_port, str, sizeof(str)))) {
	  if(tmpl_port == NULL || tmpl_port->port_type == LM_PT_IN) {
	    row[m_Columns.m_col_next_name] = str;
	    row[m_Columns.m_col_next_object_type] = LM_TYPE_GATE_PORT;
	    row[m_Columns.m_col_next_object_ptr] = (object_ptr_t *)gate_port;
	  }
	  else {
	    row[m_Columns.m_col_prev_name] = str;
	    row[m_Columns.m_col_prev_object_type] = LM_TYPE_GATE_PORT;
	    row[m_Columns.m_col_prev_object_ptr] = (object_ptr_t *)gate_port;
	  }
	}
      
	//row[m_Columns.color_] = conn->obj_ptr != src_curr_obj ? "black" : MY_GREY;
	break;
      default:
	break;
      }
    }

    conn = conn->next;
  }

}

void ConnectionInspectorWin::set_gate_port(lmodel_gate_port_t * gate_port) {
  assert(gate_port != NULL);
  if(gate_port != NULL) {
    char str[100];
    if(RET_IS_OK(lmodel_get_printable_string_for_obj(LM_TYPE_GATE_PORT, gate_port, str, sizeof(str)))) {
      current_object_label->set_text(str);
    }
    current_object_type_label->set_text("Gate port");
    clear_list();
    show_connections(LM_TYPE_GATE_PORT, (object_ptr_t *)gate_port);
  }
}

void ConnectionInspectorWin::set_wire(lmodel_wire_t * wire) {
  assert(wire != NULL);
  if(wire != NULL) {
    char str[100];
    if(RET_IS_OK(lmodel_get_printable_string_for_obj(LM_TYPE_WIRE, wire, str, sizeof(str)))) {
      current_object_label->set_text(str);
    }
    current_object_type_label->set_text("Wire");
    clear_list();
    show_connections(LM_TYPE_WIRE, (object_ptr_t *)wire);
  }
}

void ConnectionInspectorWin::set_via(lmodel_via_t * via) {
  assert(via != NULL);
  if(via != NULL) {
    char str[100];
    if(RET_IS_OK(lmodel_get_printable_string_for_obj(LM_TYPE_VIA, via, str, sizeof(str)))) {
      current_object_label->set_text(str);
    }
    current_object_type_label->set_text("Via");
    clear_list();
    show_connections(LM_TYPE_VIA, (object_ptr_t *)via);
  }
}

void ConnectionInspectorWin::set_gate(lmodel_gate_t * gate) {
  assert(gate != NULL);
  if(gate != NULL) {
    char str[100];
    if(RET_IS_OK(lmodel_get_printable_string_for_obj(LM_TYPE_GATE, gate, str, sizeof(str)))) {
      current_object_label->set_text(str);
    }
    current_object_type_label->set_text("Gate");
    clear_list();
    
    // XXX if I rewrite the logic model in c++, an iterator would help
    lmodel_gate_port_t * gate_port = gate->ports;
    while(gate_port != NULL) {
      show_connections(LM_TYPE_GATE_PORT, (object_ptr_t *)gate_port);
      gate_port = gate_port->next;
    }
    
  }
}

void ConnectionInspectorWin::set_object(LM_OBJECT_TYPE obj_type, object_ptr_t * obj_ptr) {
  lmodel_gate_t * gate = NULL;

  assert(obj_ptr != NULL);
  if(obj_ptr != NULL) {
    std::pair< LM_OBJECT_TYPE, object_ptr_t *> o(obj_type, obj_ptr);

    switch(obj_type) {
    case LM_TYPE_WIRE:
      set_wire((lmodel_wire_t *) obj_ptr);
      break;
    case LM_TYPE_VIA:
      set_via((lmodel_via_t *) obj_ptr);
      break;
    case LM_TYPE_GATE_PORT:
      set_gate_port((lmodel_gate_port_t *) obj_ptr);
      o.first = LM_TYPE_GATE_PORT;
      o.second = obj_ptr;
      break;
    case LM_TYPE_GATE:
      gate = (lmodel_gate_t *) obj_ptr;
      set_gate(gate);
      o.first = LM_TYPE_GATE;
      o.second = (object_ptr_t *)gate;
      break;
    default:
      clear_list();
      return;
    }

    back_list.push_back(o);

  }
}

void ConnectionInspectorWin::clear_list() {
  refListStore->clear();
  pGotoButton->set_sensitive(false);
}

void ConnectionInspectorWin::disable_inspection() {
  current_object_label->set_text("---");
  current_object_type_label->set_text("---");
  pGotoButton->set_sensitive(false);
  pBackButton->set_sensitive(false);
  clear_list();
  back_list.clear();

}

void ConnectionInspectorWin::on_close_button_clicked() {
  pDialog->hide();
}

void ConnectionInspectorWin::on_back_button_clicked() {
  if(back_list.size() > 1) {

    back_list.pop_back(); // remove current object

    std::pair< LM_OBJECT_TYPE, object_ptr_t *> o = back_list.back();
    back_list.pop_back();

    if(back_list.size() == 0) pBackButton->set_sensitive(false);

    set_object(o.first, o.second); // adds current obj to back_list

    signal_goto_button_clicked_(o.first, o.second);
  }
}

void ConnectionInspectorWin::on_goto_button_clicked() {
  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =  pTreeView->get_selection();
  if(refTreeSelection) {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(*iter) {
      Gtk::TreeModel::Row row = *iter;

      LM_OBJECT_TYPE object_type;
      object_ptr_t * object_ptr;

      if(row[m_Columns.m_col_next_object_ptr] != NULL) {
	object_ptr = row[m_Columns.m_col_next_object_ptr];
	object_type = row[m_Columns.m_col_next_object_type];
      }
      else {
	object_ptr = row[m_Columns.m_col_prev_object_ptr];
	object_type = row[m_Columns.m_col_prev_object_type];
      }

      pBackButton->set_sensitive(true);
      if(object_type == LM_TYPE_GATE_PORT) {
	object_type = LM_TYPE_GATE;
	object_ptr = (object_ptr_t*)((lmodel_gate_port_t *)object_ptr)->gate;
      }
      
      set_object(object_type, object_ptr);

      signal_goto_button_clicked_(object_type, object_ptr);
    }
  }
}

sigc::signal<void, LM_OBJECT_TYPE, object_ptr_t * >& 
ConnectionInspectorWin::signal_goto_button_clicked() {
  return signal_goto_button_clicked_;
}


void ConnectionInspectorWin::objects_removed() {
  disable_inspection();
}

void ConnectionInspectorWin::object_removed(LM_OBJECT_TYPE obj_type, object_ptr_t * obj_ptr) {
  disable_inspection();
}
