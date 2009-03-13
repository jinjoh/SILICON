#ifndef __SETNAMEWIN_H__
#define __SETNAMEWIN_H__

#include <gtkmm.h>

#include "lib/project.h"
#include "lib/logic_model.h"

class SetNameWin  {

 public:
  SetNameWin(Gtk::Window *parent, Glib::ustring name);
  virtual ~SetNameWin();
        
  Glib::ustring run();

 private:
  Glib::ustring orig_name;
  Gtk::Window *parent;
  Gtk::Dialog* pDialog;
  Gtk::Entry * entry;
  bool ok_clicked;

  // Signal handlers:
  virtual void on_ok_button_clicked();
  virtual void on_cancel_button_clicked();

};

#endif
