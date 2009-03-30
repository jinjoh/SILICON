#ifndef __GATESELECTWIN_H__
#define __GATESELECTWIN_H__

#include <gtkmm.h>

#include "lib/project.h"
#include "lib/logic_model.h"

class GateSelectWin  {

  class GateListModelColumns : public Gtk::TreeModelColumnRecord {
  public:
    
    GateListModelColumns() { 
      add(m_col_id); 
      add(m_col_refcount);
      add(m_col_width);
      add(m_col_height);
      add(m_col_short_name); 
      add(m_col_description); 
    }
    
    Gtk::TreeModelColumn<int> m_col_id;
    Gtk::TreeModelColumn<int> m_col_refcount;
    Gtk::TreeModelColumn<int> m_col_width;
    Gtk::TreeModelColumn<int> m_col_height;
    Gtk::TreeModelColumn<Glib::ustring> m_col_short_name;
    Gtk::TreeModelColumn<Glib::ustring> m_col_description;

  };

 public:
  GateSelectWin(Gtk::Window *parent, logic_model_t * const lmodel);

  virtual ~GateSelectWin();
        
  lmodel_gate_template_t * get_single();
  lmodel_gate_template_set_t * get_multiple();

 private:
  Gtk::Window *parent;
  logic_model_t * lmodel;
  Gtk::Dialog* pDialog;
  Gtk::Button* pOkButton;

  GateListModelColumns m_Columns;
  Glib::RefPtr<Gtk::ListStore> refListStore;
  Gtk::TreeView* pTreeView;

  bool ok_clicked;

  // Signal handlers:
  virtual void on_ok_button_clicked();
  virtual void on_cancel_button_clicked();
  virtual void on_selection_changed();
};

#endif
