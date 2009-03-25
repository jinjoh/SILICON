#ifndef __GATECONFIGWIN_H__
#define __GATECONFIGWIN_H__

#include <gtkmm.h>

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
  logic_model_t * lmodel;
  lmodel_gate_template_t * gate_template;
  unsigned int port_counter;
  
  Gtk::Dialog* pDialog;
  GateConfigModelColumns m_Columns;
  Glib::RefPtr<Gtk::ListStore> refListStore_out_ports, refListStore_in_ports;

  Gtk::TreeView* pTreeView_in_ports;
  Gtk::TreeView* pTreeView_out_ports;
  bool result;

  Gtk::Entry * entry_short_name;
  Gtk::Entry * entry_description;
  
  // Signal handlers:
  virtual void on_ok_button_clicked();
  virtual void on_cancel_button_clicked();

  virtual void on_inport_add_button_clicked();
  virtual void on_inport_remove_button_clicked();
  virtual void on_outport_add_button_clicked();
  virtual void on_outport_remove_button_clicked();


};

#endif
