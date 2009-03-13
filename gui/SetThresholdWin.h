#ifndef __SETTHRESHOLDWIN_H__
#define __SETTHRESHOLDWIN_H__

#include <gtkmm.h>

#include "lib/project.h"
#include "lib/logic_model.h"

class SetThresholdWin  {

 public:
  SetThresholdWin(Gtk::Window *parent, double threshold);
  virtual ~SetThresholdWin();
        
  double run();

 private:
  double orig_threshold;
  Gtk::Window *parent;
  Gtk::Dialog* pDialog;
  Gtk::Entry * entry;
  bool ok_clicked;

  // Signal handlers:
  virtual void on_ok_button_clicked();
  virtual void on_cancel_button_clicked();

};

#endif
