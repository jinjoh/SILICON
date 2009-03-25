#ifndef __CONNECTIONINSPECTORWIN_H__
#define __CONNECTIONINSPECTORWIN_H__

#include <gtkmm.h>
#include "lib/logic_model.h"

class ConnectionInspectorWin : public Gtk::Window {

  class ConnectionInspectorModelColumns : public Gtk::TreeModelColumnRecord {
  public:
    
    ConnectionInspectorModelColumns() { 
      add(m_col_id); 
      add(m_col_sub_id); 
      add(m_col_object_type_name); 

      add(m_col_parent); 
      add(m_col_name);

      add(m_col_object_type);
      add(m_col_object_ptr);
    }
    
    Gtk::TreeModelColumn<int> m_col_id;
    Gtk::TreeModelColumn<int> m_col_sub_id;
    Gtk::TreeModelColumn<Glib::ustring> m_col_object_type_name;
    Gtk::TreeModelColumn<Glib::ustring> m_col_parent;
    Gtk::TreeModelColumn<Glib::ustring> m_col_name;

    Gtk::TreeModelColumn<LM_OBJECT_TYPE> m_col_object_type;
    Gtk::TreeModelColumn<object_ptr_t * > m_col_object_ptr;

  };


 public:
  ConnectionInspectorWin(Gtk::Window *parent, logic_model_t * lmodel);
  virtual ~ConnectionInspectorWin();
  void set_gate_port(lmodel_gate_port_t * gate_port);
  void set_wire(lmodel_wire_t * wire);
  void set_via(lmodel_via_t * via);

  void disable_inspection();
  void show();

  sigc::signal<void, LM_OBJECT_TYPE, object_ptr_t *>& signal_goto_button_clicked();

 private:
  Gtk::Window *parent;
  logic_model_t * lmodel;

  Gtk::Dialog * pDialog;

  Gtk::Button* pGotoButton;
  Gtk::Button* pCloseButton;


  ConnectionInspectorModelColumns m_Columns;
  Glib::RefPtr<Gtk::ListStore> refListStore;
  Gtk::TreeView* pTreeView;

  Gtk::Label * current_object_label;
  Gtk::Label * current_object_type_label;

  sigc::signal<void, LM_OBJECT_TYPE, object_ptr_t *>  signal_goto_button_clicked_;

  void clear_list();
  void show_connections(object_ptr_t * curr_obj, lmodel_connection_t * connections);

  // Signal handlers:
  virtual void on_close_button_clicked();
  virtual void on_goto_button_clicked();

  virtual void on_selection_changed();
};

#endif
