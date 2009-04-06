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

#ifndef __PORTSELECTWIN_H__
#define __PORTSELECTWIN_H__

#include <gtkmm.h>

#include "lib/project.h"
#include "lib/logic_model.h"

class PortSelectWin  {

  class PortListModelColumns : public Gtk::TreeModelColumnRecord {
  public:
    
    PortListModelColumns() { 
      add(m_col_id); 
      add(m_col_name); 
    }
    
    Gtk::TreeModelColumn<int> m_col_id;
    Gtk::TreeModelColumn<Glib::ustring> m_col_name;
  };

 public:
  PortSelectWin(Gtk::Window *parent, lmodel_gate_t * gate);

  virtual ~PortSelectWin();
  
  lmodel_gate_template_port_t * run();

 private:
  Gtk::Window *parent;
  lmodel_gate_t * gate;
  lmodel_gate_template_port_t * template_port;
  Gtk::Dialog* pDialog;

  PortListModelColumns m_Columns;
  Glib::RefPtr<Gtk::ListStore> refListStore;
  Gtk::TreeView* pTreeView;

  // Signal handlers:
  virtual void on_ok_button_clicked();
  virtual void on_cancel_button_clicked();

};

#endif
