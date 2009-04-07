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

#include "GateListWin.h"
#include "GateConfigWin.h"
#include "lib/grid.h"

#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include <assert.h>
#include <stdlib.h>

GateListWin::GateListWin(Gtk::Window *parent, logic_model_t * lmodel) {

  this->lmodel = lmodel;
  this->parent = parent;

  char file[PATH_MAX];
  snprintf(file, PATH_MAX, "%s/glade/gate_list.glade", getenv("DEGATE_HOME"));

  assert(lmodel);
  //assert(lmodel->gate_set);

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
    refXml->get_widget("gate_list_dialog", pDialog);
    if(pDialog) {
      //Get the Glade-instantiated Button, and connect a signal handler:
      Gtk::Button* pButton = NULL;

      // connect signals
      refXml->get_widget("close_button", pButton);
      if(pButton)
 	pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateListWin::on_close_button_clicked));

      refXml->get_widget("add_button", pButton);
      if(pButton)
	pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateListWin::on_add_button_clicked) );

      refXml->get_widget("remove_button", pButton);
      if(pButton)
	pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateListWin::on_remove_button_clicked) );

      refXml->get_widget("edit_button", pButton);
      if(pButton)
	pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateListWin::on_edit_button_clicked) );

      refListStore = Gtk::ListStore::create(m_Columns);

      refXml->get_widget("treeview", pTreeView);
      if(pTreeView) {
	pTreeView->set_model(refListStore);
	pTreeView->append_column("ID", m_Columns.m_col_id);
	pTreeView->append_column("#", m_Columns.m_col_refcount);
	pTreeView->append_column("Width", m_Columns.m_col_width);
	pTreeView->append_column("Height", m_Columns.m_col_height);
	pTreeView->append_column("Short Name", m_Columns.m_col_short_name);
	pTreeView->append_column("Description", m_Columns.m_col_description);

	Gtk::TreeView::Column * pColumn;

	pColumn = pTreeView->get_column(0);
	if(pColumn) pColumn->set_sort_column(m_Columns.m_col_id);
	
	pColumn = pTreeView->get_column(1);
	if(pColumn) pColumn->set_sort_column(m_Columns.m_col_refcount);
	
	pColumn = pTreeView->get_column(2);
	if(pColumn) pColumn->set_sort_column(m_Columns.m_col_width);
	
	pColumn = pTreeView->get_column(3);
	if(pColumn) pColumn->set_sort_column(m_Columns.m_col_height);
	
	pColumn = pTreeView->get_column(4);
	if(pColumn) pColumn->set_sort_column(m_Columns.m_col_short_name);
	
	pColumn = pTreeView->get_column(5);
	if(pColumn) pColumn->set_sort_column(m_Columns.m_col_description);

	Gtk::CellRendererText * pRenderer = Gtk::manage( new Gtk::CellRendererText()); 
	pTreeView->append_column("Fill color", *pRenderer);
	pColumn = pTreeView->get_column(6);
	pColumn->add_attribute(*pRenderer, "background-gdk", m_Columns.color_fill_); 
	pColumn->add_attribute(*pRenderer, "xpad", m_Columns.padding_); 
	pColumn->add_attribute(*pRenderer, "ypad", m_Columns.padding_); 

	pRenderer = Gtk::manage( new Gtk::CellRendererText()); 
	pTreeView->append_column("Frame color", *pRenderer);
	pColumn = pTreeView->get_column(7);
	pColumn->add_attribute(*pRenderer, "background-gdk", m_Columns.color_frame_); 
	pColumn->add_attribute(*pRenderer, "xpad", m_Columns.padding_); 
	pColumn->add_attribute(*pRenderer, "ypad", m_Columns.padding_); 


      }

      lmodel_gate_template_set_t * ptr = lmodel->gate_template_set;
      while(ptr != NULL) {

	if(ptr->gate) {
	  Gtk::TreeModel::Row row = *(refListStore->append()); 

	  row[m_Columns.m_col_id] = ptr->gate->id;
	  row[m_Columns.m_col_refcount] = ptr->gate->reference_counter;

	  row[m_Columns.m_col_refcount] = ptr->gate->reference_counter;
	  row[m_Columns.m_col_width] = ptr->gate->master_image_max_x - ptr->gate->master_image_min_x;
	  row[m_Columns.m_col_height] = ptr->gate->master_image_max_y - ptr->gate->master_image_min_y;

	  if(ptr->gate->short_name) row[m_Columns.m_col_short_name] = ptr->gate->short_name;
	  if(ptr->gate->description) row[m_Columns.m_col_description] = ptr->gate->description;

	  row[m_Columns.color_fill_] = get_fill_color(ptr->gate);
	  row[m_Columns.color_frame_] = get_frame_color(ptr->gate);
	  row[m_Columns.padding_] = 5;
	}

	ptr = ptr->next;
      }

    }
    else {
      std::cout << "Error: can't find gate_list_dialog" << std::endl;
    }

}

Gdk::Color GateListWin::get_fill_color(lmodel_gate_template_t * tmpl) {

  Gdk::Color c;
  color_t frame_color, fill_color;
  if(RET_IS_NOT_OK(lmodel_gate_template_get_color(tmpl, &fill_color, &frame_color))) {
    debug(TM, "Can't get color definitions");
  }

  if(fill_color != 0) {
    c.set_red(MASK_R(fill_color) << 8);
    c.set_green(MASK_G(fill_color) << 8);
    c.set_blue(MASK_B(fill_color) << 8);
  }
  else {
    c.set_red(0x30 << 8);
    c.set_green(0x30 << 8);
    c.set_blue(0x30 << 8);
  }

  return c;
}

Gdk::Color GateListWin::get_frame_color(lmodel_gate_template_t * tmpl) {

  Gdk::Color c;
  color_t frame_color, fill_color;
  if(RET_IS_NOT_OK(lmodel_gate_template_get_color(tmpl, &fill_color, &frame_color))) {
    debug(TM, "Can't get color definitions");
  }

  if(frame_color != 0) {
    c.set_red(MASK_R(frame_color) << 8);
    c.set_green(MASK_G(frame_color) << 8);
    c.set_blue(MASK_B(frame_color) << 8);
  }
  else {
    c.set_red(0xa0 << 8);
    c.set_green(0xa0 << 8);
    c.set_blue(0xa0 << 8);
  }

  return c;
}

GateListWin::~GateListWin() {
  delete pDialog;
}

void GateListWin::run() {
  
  pDialog->run();
}

void GateListWin::on_close_button_clicked() {
  pDialog->hide();
}

void GateListWin::on_add_button_clicked() {

  lmodel_gate_template_t * tmpl = lmodel_create_gate_template();

  GateConfigWin gcWin(parent, lmodel, tmpl);
  if(gcWin.run() == true) {
    if(RET_IS_OK(lmodel_add_gate_template(lmodel, tmpl, 0))) {
      Gtk::TreeModel::Row row = *(refListStore->append()); 
      
      row[m_Columns.m_col_id] = tmpl->id;
      row[m_Columns.m_col_refcount] = tmpl->reference_counter;
      row[m_Columns.m_col_width] = tmpl->master_image_max_x - tmpl->master_image_min_x;
      row[m_Columns.m_col_height] = tmpl->master_image_max_y - tmpl->master_image_min_y;
      row[m_Columns.m_col_short_name] = tmpl->short_name;
      row[m_Columns.m_col_description] = tmpl->description;

      row[m_Columns.color_fill_] = get_fill_color(tmpl);
      row[m_Columns.color_frame_] = get_frame_color(tmpl);

    }
  }

}

void GateListWin::on_remove_button_clicked() {
  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =  pTreeView->get_selection();
  if(refTreeSelection) {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(iter) {
      Gtk::TreeModel::Row row = *iter; 
      int obj_id = row[m_Columns.m_col_id];

      Gtk::MessageDialog dialog(*parent, "Are you sure you want to remove a gate template?", 
				true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
      dialog.set_title("Warning");      
      if(dialog.run() == Gtk::RESPONSE_YES) {
	dialog.hide();

	lmodel_gate_template_t * tmpl = lmodel_get_gate_template_by_id(lmodel, obj_id);
	if(tmpl) {

	  if(tmpl->reference_counter > 0) {
	    Gtk::MessageDialog dialog2(*parent, 
				       "The template is referenced by placed gates. "
				       "Do you want to remove the gates as well?", 
				       true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);

	    if(dialog2.run() == Gtk::RESPONSE_YES) {
	      dialog2.hide();
	      if(RET_IS_NOT_OK(lmodel_destroy_gates_by_template_type(lmodel, tmpl, DESTROY_ALL))) {
		Gtk::MessageDialog dialog(*parent, "Can't remove gate.", true, Gtk::MESSAGE_ERROR);
		dialog.set_title("Error");
		dialog.run();
	      }
	    }
	  }

	  if(RET_IS_NOT_OK(lmodel_remove_gate_template(lmodel, tmpl))) {
	    Gtk::MessageDialog dialog(*parent, "Can't remove template gate.", true, Gtk::MESSAGE_ERROR);
	    dialog.set_title("Error");
	    dialog.run();
	  }
	  else 
	    refListStore->erase(iter);
	}
	
      }
    }

  }
}

void GateListWin::on_edit_button_clicked() {

  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =  pTreeView->get_selection();
  if(refTreeSelection) {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(iter) {
      Gtk::TreeModel::Row row = *iter; 
      int obj_id = row[m_Columns.m_col_id];
      lmodel_gate_template_t * tmpl = lmodel_get_gate_template_by_id(lmodel, obj_id);

      GateConfigWin gcWin(parent, lmodel, tmpl);
      if(gcWin.run() == true) {
	row[m_Columns.m_col_id] = tmpl->id;
	row[m_Columns.m_col_refcount] = tmpl->reference_counter;
	row[m_Columns.m_col_width] = tmpl->master_image_max_x - tmpl->master_image_min_x;
	row[m_Columns.m_col_height] = tmpl->master_image_max_y - tmpl->master_image_min_y;
	row[m_Columns.m_col_short_name] = tmpl->short_name;
	row[m_Columns.m_col_description] = tmpl->description;

	row[m_Columns.color_fill_] = get_fill_color(tmpl);
	row[m_Columns.color_frame_] = get_frame_color(tmpl);
      }
    }
  }
}

