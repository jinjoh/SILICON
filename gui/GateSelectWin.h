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
      add(m_col_short_name); 
      add(m_col_description); 
    }
    
    Gtk::TreeModelColumn<int> m_col_id;
    Gtk::TreeModelColumn<Glib::ustring> m_col_short_name;
    Gtk::TreeModelColumn<Glib::ustring> m_col_description;

  };

 public:
  GateSelectWin(Gtk::Window *parent, logic_model_t * const lmodel);

  virtual ~GateSelectWin();
        
  lmodel_gate_template_t * run();

 private:
  Gtk::Window *parent;
  logic_model_t * lmodel;
  Gtk::Dialog* pDialog;
  lmodel_gate_template_t * result;

  GateListModelColumns m_Columns;
  Glib::RefPtr<Gtk::ListStore> refListStore;
  Gtk::TreeView* pTreeView;

  // Signal handlers:
  virtual void on_ok_button_clicked();
  virtual void on_cancel_button_clicked();

};

#endif
