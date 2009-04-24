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

#ifndef __GATECONFIGWIN_H__
#define __GATECONFIGWIN_H__

#include <gtkmm.h>
#include <list>

#include "lib/project.h"
#include "lib/logic_model.h"

class GateConfigWin  {

  class GateConfigModelColumns : public Gtk::TreeModelColumnRecord {
  public:
    
    GateConfigModelColumns() { 
      add(m_col_id); 
      add(m_col_text); 
    }
    
    Gtk::TreeModelColumn<int> m_col_id;
    Gtk::TreeModelColumn<Glib::ustring> m_col_text;
  };

 public:
  GateConfigWin(Gtk::Window *parent, 
		logic_model_t * const lmodel, 
		lmodel_gate_template_t * const gate_template);

  virtual ~GateConfigWin();
        
  bool run();

  private:
  Gtk::Window *parent;

  logic_model_t * lmodel;
  lmodel_gate_template_t * gate_template;
  unsigned int port_counter;

  std::list<unsigned int> original_ports;

  Gtk::Dialog* pDialog;
  GateConfigModelColumns m_Columns;
  Glib::RefPtr<Gtk::ListStore> refListStore_out_ports, refListStore_in_ports;

  Gtk::TreeView* pTreeView_in_ports;
  Gtk::TreeView* pTreeView_out_ports;
  bool result;

  Gtk::Entry * entry_short_name;
  Gtk::Entry * entry_description;

  Gtk::ColorButton * colorbutton_fill_color;
  Gtk::ColorButton * colorbutton_frame_color;

  // Signal handlers:
  virtual void on_ok_button_clicked();
  virtual void on_cancel_button_clicked();

  virtual void on_inport_add_button_clicked();
  virtual void on_inport_remove_button_clicked();
  virtual void on_outport_add_button_clicked();
  virtual void on_outport_remove_button_clicked();


};

#endif
