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

#include "SetOrientationWin.h"

#include <assert.h>
#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include <stdlib.h>

SetOrientationWin::SetOrientationWin(Gtk::Window *parent, LM_TEMPLATE_ORIENTATION orientation ) {

  assert(parent);
  this->parent = parent;
  this->orig_orientation = orientation;
  ok_clicked = false;

  char file[PATH_MAX];
  snprintf(file, PATH_MAX, "%s/glade/set_orientation.glade", getenv("DEGATE_HOME"));

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
  refXml->get_widget("set_orientation_dialog", pDialog);
  if(pDialog) {
    //Get the Glade-instantiated Button, and connect a signal handler:
    Gtk::Button* pButton = NULL;
    
    // connect signals
    refXml->get_widget("cancel_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &SetOrientationWin::on_cancel_button_clicked));
    
    refXml->get_widget("ok_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &SetOrientationWin::on_ok_button_clicked) );
  
    refXml->get_widget("combobox1", entry);
    if(entry) {

      m_refTreeModel = Gtk::ListStore::create(m_Columns);
      entry->set_model(m_refTreeModel);

      Gtk::TreeModel::Row row = *(m_refTreeModel->append());
      row[m_Columns.m_col_id] = LM_TEMPLATE_ORIENTATION_UNDEFINED;
      row[m_Columns.m_col_name] = "undefined";

      row = *(m_refTreeModel->append());
      row[m_Columns.m_col_id] = LM_TEMPLATE_ORIENTATION_NORMAL;
      row[m_Columns.m_col_name] = "normal";

      row = *(m_refTreeModel->append());
      row[m_Columns.m_col_id] = LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN;
      row[m_Columns.m_col_name] = "flipped up-down";

      row = *(m_refTreeModel->append());
      row[m_Columns.m_col_id] = LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT;
      row[m_Columns.m_col_name] = "flipped left-right";

      row = *(m_refTreeModel->append());
      row[m_Columns.m_col_id] = LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH;
      row[m_Columns.m_col_name] = "flipped left-right and up-down";

      entry->pack_start(m_Columns.m_col_name);
      entry->set_active(orientation);
    }

  }
}

SetOrientationWin::~SetOrientationWin() {
  delete pDialog;
}


LM_TEMPLATE_ORIENTATION SetOrientationWin::run() {
  pDialog->run();
  if(ok_clicked) return orientation;
  else return orig_orientation;
}

void SetOrientationWin::on_ok_button_clicked() {
  ok_clicked = true;

  Gtk::TreeModel::iterator iter = entry->get_active();
  if(iter) {
    Gtk::TreeModel::Row row = *iter;
    if(row) {
      int id = row[m_Columns.m_col_id];
      switch(id) {
      case LM_TEMPLATE_ORIENTATION_UNDEFINED:
	orientation = LM_TEMPLATE_ORIENTATION_UNDEFINED;
	break;
      case LM_TEMPLATE_ORIENTATION_NORMAL:
	orientation = LM_TEMPLATE_ORIENTATION_NORMAL;
	break;
      case LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN:
	orientation = LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN;
	break;
      case LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT:
	orientation = LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT;
	break;
      case LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH:
	orientation = LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH;
	break;
	
      }
      pDialog->hide();
    }
  }
}

void SetOrientationWin::on_cancel_button_clicked() {
  ok_clicked = false;
  pDialog->hide();
}

