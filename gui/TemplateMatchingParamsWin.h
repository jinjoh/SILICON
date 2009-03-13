#ifndef __TEMPLATEMATCHINGPARAMSWIN_H__
#define __TEMPLATEMATCHINGPARAMSWIN_H__

#include <gtkmm.h>

#include "lib/project.h"
#include "lib/logic_model.h"

class TemplateMatchingParamsWin  {

 public:
  TemplateMatchingParamsWin(Gtk::Window *parent, 
			    double threshold,
			    unsigned int max_step_size_search,
			    unsigned int max_step_size_xcorr);
  virtual ~TemplateMatchingParamsWin();
        
  ret_t run(double * threshold,
	    unsigned int * max_step_size_search,
	    unsigned int * max_step_size_xcorr);

 private:
  double orig_threshold;
  Gtk::Window *parent;
  Gtk::Dialog* pDialog;

  Gtk::HScale * hscale_threshold;
  Gtk::Entry * entry_step_size_search;
  Gtk::Entry * entry_step_size_xcorr;

  bool ok_clicked;

  // Signal handlers:
  virtual void on_ok_button_clicked();
  virtual void on_cancel_button_clicked();

};

#endif
