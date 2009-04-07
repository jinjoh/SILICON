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

#ifndef __PORTCOLORSWIN_H__
#define __PORTCOLORSWIN_H__

#include <gtkmm.h>
#include "lib/logic_model.h"
#include "lib/port_color_manager.h"

class PortColorsWin {

  class PortColorsModelColumns : public Gtk::TreeModelColumnRecord {
  public:
    
    PortColorsModelColumns() { 
      add(m_col_port_name); 
      add(color_);
    }
    
    Gtk::TreeModelColumn<Glib::ustring> m_col_port_name;
    Gtk::TreeModelColumn<Gdk::Color> color_;
  };


 public:
  PortColorsWin(Gtk::Window *parent, logic_model_t * lmodel, port_color_manager_t * pcm);
  virtual ~PortColorsWin();
  void run();

 private:
  Gtk::Window *parent;
  logic_model_t * lmodel;
  port_color_manager_t * pcm;

  Gtk::Button* pEditButton;
  Gtk::Button* pRemoveButton;

  Gtk::Dialog * pDialog;
  PortColorsModelColumns m_Columns;
  Glib::RefPtr<Gtk::ListStore> refListStore;
  Gtk::TreeView* pTreeView;

  Gdk::Color get_color(color_t col);
  void apply_colors();

  // Signal handlers:
  virtual void on_close_button_clicked();
  virtual void on_add_button_clicked();
  virtual void on_remove_button_clicked();
  virtual void on_edit_button_clicked();

  virtual void on_selection_changed();
};

#endif
