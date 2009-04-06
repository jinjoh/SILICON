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

#ifndef __SETORIENTATIONWIN_H__
#define __SETORIENTATIONWIN_H__

#include <gtkmm.h>

#include "lib/project.h"
#include "lib/logic_model.h"

class SetOrientationWin  {


 public:

  class ModelColumns : public Gtk::TreeModel::ColumnRecord {
  public:
    
    ModelColumns()
      { add(m_col_id); add(m_col_name); }
    
    Gtk::TreeModelColumn<int> m_col_id;
    Gtk::TreeModelColumn<Glib::ustring> m_col_name;
  };
  ModelColumns m_Columns;


  SetOrientationWin(Gtk::Window *parent, LM_TEMPLATE_ORIENTATION orientation);
  virtual ~SetOrientationWin();
        
  LM_TEMPLATE_ORIENTATION run();

 private:
  LM_TEMPLATE_ORIENTATION orientation;
  LM_TEMPLATE_ORIENTATION orig_orientation;

  Gtk::Window *parent;
  Gtk::Dialog* pDialog;
  Gtk::ComboBox * entry;
  bool ok_clicked;
  Glib::RefPtr<Gtk::ListStore> m_refTreeModel;

  // Signal handlers:
  virtual void on_ok_button_clicked();
  virtual void on_cancel_button_clicked();

};

#endif
