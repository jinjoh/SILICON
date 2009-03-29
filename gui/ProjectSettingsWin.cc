#include "ProjectSettingsWin.h"

#include <assert.h>
#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include <stdlib.h>

#define TM "ProjectSettingsWin.cc"

ProjectSettingsWin::ProjectSettingsWin(Gtk::Window *parent, project_t * project) {

  assert(parent);
  this->parent = parent;
  ok_clicked = false;
  this->project = project;

  char file[PATH_MAX];
  snprintf(file, PATH_MAX, "%s/glade/project_settings.glade", getenv("DEGATE_HOME"));

  //Load the Glade file and instiate its widgets:
  Glib::RefPtr<Gnome::Glade::Xml> refXml;
#ifdef GLIBMM_EXCEPTIONS_ENABLED
  try {
    refXml = Gnome::Glade::Xml::create(file);
  }
  catch(const Gnome::Glade::XmlError& ex) {
    std::cerr << ex.what() << std::endl;
    return;
  }
#else
  std::auto_ptr<Gnome::Glade::XmlError> error;
  refXml = Gnome::Glade::Xml::create(file, "", "", error);
  if(error.get()) {
    std::cerr << error->what() << std::endl;
    return;
  }
#endif

  //Get the Glade-instantiated Dialog:
  refXml->get_widget("project_settings_dialog", pDialog);
  if(pDialog) {
    //Get the Glade-instantiated Button, and connect a signal handler:
    Gtk::Button* pButton = NULL;
    
    // connect signals
    refXml->get_widget("cancel_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &ProjectSettingsWin::on_cancel_button_clicked));
    
    refXml->get_widget("ok_button", pButton);
    if(pButton) {
      pButton->grab_focus();
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &ProjectSettingsWin::on_ok_button_clicked) );
    }

    char str[100];

    refXml->get_widget("entry_lambda", entry_lambda);
    if(entry_lambda) {
      snprintf(str, sizeof(str), "%d", project->lambda);
      entry_lambda->set_text(str);
    }
    refXml->get_widget("entry_via_diameter", entry_via_diameter);
    if(entry_via_diameter) {
      snprintf(str, sizeof(str), "%d", project->pin_diameter);
      entry_via_diameter->set_text(str);
    }
    refXml->get_widget("entry_wire_diameter", entry_wire_diameter);
    if(entry_wire_diameter) {
      snprintf(str, sizeof(str), "%d", project->wire_diameter);
      entry_wire_diameter->set_text(str);
    }

  }
}

ProjectSettingsWin::~ProjectSettingsWin() {
  delete pDialog;
}


bool ProjectSettingsWin::run() {
  pDialog->run();
  if(ok_clicked) {
    long r;
    if((r = atol(entry_lambda->get_text().c_str())) > 0)
      project->lambda = r;
    if((r = atol(entry_via_diameter->get_text().c_str())) > 0)
      project->pin_diameter = r;
    if((r = atol(entry_wire_diameter->get_text().c_str())) > 0)
      project->wire_diameter = r;
    return true;
  }
  else return false;
}

void ProjectSettingsWin::on_ok_button_clicked() {
  ok_clicked = true;
  pDialog->hide();
}

void ProjectSettingsWin::on_cancel_button_clicked() {
  ok_clicked = false;
  pDialog->hide();
}

