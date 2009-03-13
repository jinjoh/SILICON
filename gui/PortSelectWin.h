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
  
  gate_template_port_t * run();

 private:
  Gtk::Window *parent;
  lmodel_gate_t * gate;
  gate_template_port_t * template_port;
  Gtk::Dialog* pDialog;

  PortListModelColumns m_Columns;
  Glib::RefPtr<Gtk::ListStore> refListStore;
  Gtk::TreeView* pTreeView;

  // Signal handlers:
  virtual void on_ok_button_clicked();
  virtual void on_cancel_button_clicked();

};

#endif
