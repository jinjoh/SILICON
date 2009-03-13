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
