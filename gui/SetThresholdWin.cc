#include "SetThresholdWin.h"

#include <assert.h>
#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include <stdlib.h>

#define TM "SetThresholdWin.cc"

SetThresholdWin::SetThresholdWin(Gtk::Window *parent, double threshold) {

  assert(parent);
  this->parent = parent;
  ok_clicked = false;
  orig_threshold = threshold;

  char file[PATH_MAX];
  snprintf(file, PATH_MAX, "%s/glade/set_threshold.glade", getenv("DEGATE_HOME"));

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
  refXml->get_widget("set_threshold_dialog", pDialog);
  if(pDialog) {
    //Get the Glade-instantiated Button, and connect a signal handler:
    Gtk::Button* pButton = NULL;
    
    // connect signals
    refXml->get_widget("cancel_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &SetThresholdWin::on_cancel_button_clicked));
    
    refXml->get_widget("ok_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &SetThresholdWin::on_ok_button_clicked) );
  

    refXml->get_widget("entry_name", entry);
    if(entry) {
      char txt[100];
      snprintf(txt, sizeof(txt), "%f", threshold);
      entry->set_text(strdup(txt));
    }

  }
}

SetThresholdWin::~SetThresholdWin() {
  delete pDialog;
}

double SetThresholdWin::run() {
  pDialog->run();
  if(ok_clicked) {
    const char * start_ptr = entry->get_text().c_str();
    char * end_ptr;
    double r = strtod(start_ptr, &end_ptr);
    if(r == 0 && end_ptr == start_ptr + strlen(start_ptr) + 1) {
      debug(TM, "Error");
      return orig_threshold;
    }
    else {
      debug(TM, "Threshold is %f", r);
      return r;
    }
  }
  else return orig_threshold;
}

void SetThresholdWin::on_ok_button_clicked() {
  ok_clicked = true;
  pDialog->hide();
}

void SetThresholdWin::on_cancel_button_clicked() {
  ok_clicked = false;
  pDialog->hide();
}

