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

#ifndef __CONNECTIONINSPECTORWIN_H__
#define __CONNECTIONINSPECTORWIN_H__

#include <gtkmm.h>
#include "lib/logic_model.h"

#include <list>

class ConnectionInspectorWin : public Gtk::Window {

  class ConnectionInspectorModelColumns : public Gtk::TreeModelColumnRecord {
  public:
    
    ConnectionInspectorModelColumns() { 

      add(m_col_curr_object_type);
      add(m_col_curr_object_ptr);
      add(m_col_curr_name);
      add(m_col_curr_name_sort);

      add(m_col_prev_object_type);
      add(m_col_prev_object_ptr);
      add(m_col_prev_name);

      add(m_col_next_object_type);
      add(m_col_next_object_ptr);
      add(m_col_next_name);

      add(color_);
    }
    
    Gtk::TreeModelColumn<Glib::ustring> m_col_curr_object_type_name;
    Gtk::TreeModelColumn<Glib::ustring> m_col_curr_name;
    Gtk::TreeModelColumn<Glib::ustring> m_col_curr_name_sort;
    Gtk::TreeModelColumn<LM_OBJECT_TYPE> m_col_curr_object_type;
    Gtk::TreeModelColumn<object_ptr_t * > m_col_curr_object_ptr;

    Gtk::TreeModelColumn<Glib::ustring> m_col_next_name;
    Gtk::TreeModelColumn<LM_OBJECT_TYPE> m_col_next_object_type;
    Gtk::TreeModelColumn<object_ptr_t * > m_col_next_object_ptr;

    Gtk::TreeModelColumn<Glib::ustring> m_col_prev_name;
    Gtk::TreeModelColumn<LM_OBJECT_TYPE> m_col_prev_object_type;
    Gtk::TreeModelColumn<object_ptr_t * > m_col_prev_object_ptr;

    Gtk::TreeModelColumn<Glib::ustring> color_; 

  };


 public:
  ConnectionInspectorWin(Gtk::Window *parent, logic_model_t * lmodel);
  virtual ~ConnectionInspectorWin();
  void set_object(LM_OBJECT_TYPE obj_type, object_ptr_t * obj_ptr);

  void disable_inspection();
  void show();

  void object_removed(LM_OBJECT_TYPE obj_type, object_ptr_t * obj_ptr);
  void objects_removed();

  sigc::signal<void, LM_OBJECT_TYPE, object_ptr_t *>& signal_goto_button_clicked();

 private:
  Gtk::Window *parent;
  logic_model_t * lmodel;

  std::list< std::pair< LM_OBJECT_TYPE, object_ptr_t *> > back_list;

  Gtk::Dialog * pDialog;

  Gtk::Button* pBackButton;
  Gtk::Button* pGotoButton;
  Gtk::Button* pCloseButton;


  ConnectionInspectorModelColumns m_Columns;
  Glib::RefPtr<Gtk::ListStore> refListStore;
  Gtk::TreeView* pTreeView;

  Gtk::Label * current_object_label;
  Gtk::Label * current_object_type_label;

  sigc::signal<void, LM_OBJECT_TYPE, object_ptr_t *>  signal_goto_button_clicked_;

  void clear_list();
  void show_connections(LM_OBJECT_TYPE src_object_type,
			object_ptr_t * src_curr_obj,
			Glib::ustring current_color);


  void set_gate_port(lmodel_gate_port_t * gate_port);
  void set_wire(lmodel_wire_t * wire);
  void set_via(lmodel_via_t * via);
  void set_gate(lmodel_gate_t * gate);

  // Signal handlers:
  virtual void on_close_button_clicked();
  virtual void on_goto_button_clicked();
  virtual void on_back_button_clicked();

  virtual void on_selection_changed();
};

#endif
