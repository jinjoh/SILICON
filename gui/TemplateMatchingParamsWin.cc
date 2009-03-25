#include "TemplateMatchingParamsWin.h"

#include <assert.h>
#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include <stdlib.h>

#define TM "TemplateMatchingParamsWin.cc"

TemplateMatchingParamsWin::TemplateMatchingParamsWin(Gtk::Window *parent, 
						     double threshold,
						     unsigned int max_step_size_search,
						     unsigned int max_step_size_xcorr) {

  assert(parent);
  this->parent = parent;
  ok_clicked = false;
  orig_threshold = threshold;

  char file[PATH_MAX];
  snprintf(file, PATH_MAX, "%s/glade/template_matching_params.glade", getenv("DEGATE_HOME"));

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
  refXml->get_widget("set_template_matching_params_dialog", pDialog);
  if(pDialog) {
    //Get the Glade-instantiated Button, and connect a signal handler:
    Gtk::Button* pButton = NULL;
    
    // connect signals
    refXml->get_widget("cancel_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &TemplateMatchingParamsWin::on_cancel_button_clicked));
    
    refXml->get_widget("ok_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &TemplateMatchingParamsWin::on_ok_button_clicked) );
  

    refXml->get_widget("hscale_threshold", hscale_threshold);
    if(hscale_threshold) {
      hscale_threshold->set_value(threshold);
    }

    refXml->get_widget("entry_step_size_search", entry_step_size_search);
    if(entry_step_size_search) {
      char txt[100];
      snprintf(txt, sizeof(txt), "%d", max_step_size_search);
      entry_step_size_search->set_text(strdup(txt));
    }

    refXml->get_widget("entry_step_size_xcorr", entry_step_size_xcorr);
    if(entry_step_size_xcorr) {
      char txt[100];
      snprintf(txt, sizeof(txt), "%d", max_step_size_xcorr);
      entry_step_size_xcorr->set_text(strdup(txt));
    }

  }
}

TemplateMatchingParamsWin::~TemplateMatchingParamsWin() {
  delete pDialog;
}

ret_t TemplateMatchingParamsWin::run(double * threshold,
				      unsigned int * max_step_size_search,
				      unsigned int * max_step_size_xcorr) {
  assert(threshold != NULL);
  assert(max_step_size_search != NULL);
  assert(max_step_size_xcorr != NULL);

  *max_step_size_search = 0;
  *max_step_size_xcorr = 0;

  while(*max_step_size_search == 0 || *max_step_size_xcorr == 0) {
    pDialog->run();
    if(ok_clicked) {

      *max_step_size_search = atoi(entry_step_size_search->get_text().c_str());
      *max_step_size_xcorr = atoi(entry_step_size_xcorr->get_text().c_str());
      *threshold = hscale_threshold->get_value();

      if(*max_step_size_search > 0 && *max_step_size_xcorr > 0) {
	pDialog->hide();
	return RET_OK;
      }
    }
    else return RET_CANCEL;
  }

  return RET_CANCEL;
}

void TemplateMatchingParamsWin::on_ok_button_clicked() {
  ok_clicked = true;
}

void TemplateMatchingParamsWin::on_cancel_button_clicked() {
  ok_clicked = false;
  pDialog->hide();
}

