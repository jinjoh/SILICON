#ifndef __PROJECTSETTINGSWIN_H__
#define __PROJECTSETTINGSWIN_H__

#include <gtkmm.h>

#include "lib/project.h"
#include "lib/logic_model.h"

class ProjectSettingsWin  {

 public:
  ProjectSettingsWin(Gtk::Window *parent, project_t * project);

  virtual ~ProjectSettingsWin();
        
  bool run();

 private:
  project_t * project;

  Gtk::Window *parent;
  Gtk::Dialog* pDialog;
  Gtk::Entry * entry_lambda;
  Gtk::Entry * entry_wire_diameter;
  Gtk::Entry * entry_via_diameter;

  bool ok_clicked;

  // Signal handlers:
  virtual void on_ok_button_clicked();
  virtual void on_cancel_button_clicked();

};

#endif
