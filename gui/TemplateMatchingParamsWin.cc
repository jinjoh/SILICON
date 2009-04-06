/*                                                                              
                                                                                
This file is part of the IC reverse engineering tool degate.                    
                                                                                
Copyright 2008, 2009 by Martin Schobert                                         
                                                                                
Degate is free software: you can redistribute it and/or modify                  
it under the terms of the GNU General Public License as published by            
the Free Software Foundation, either version 3 of the License, or               
any later version.                                                              
                                                                                
Degate is distributed in the hope that it will be useful,                       
but WITHOUT ANY WARRANTY; without even the implied warranty of                  
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                   
GNU General Public License for more details.                                    
                                                                                
You should have received a copy of the GNU General Public License               
along with degate. If not, see <http://www.gnu.org/licenses/>.                  
                                                                                
*/

#include "TemplateMatchingParamsWin.h"

#include <assert.h>
#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include <stdlib.h>
#include "scaling_manager.h"

TemplateMatchingParamsWin::TemplateMatchingParamsWin(Gtk::Window *parent,
						     scaling_manager_t * scaling_manager,
						     double threshold_hc,
						     double threshold_detection,
						     unsigned int max_step_size_search,
						     unsigned int preselected_scale_down) {

  assert(parent);
  this->parent = parent;
  ok_clicked = false;

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
  if(pDialog != NULL) {
    //Get the Glade-instantiated Button, and connect a signal handler:
    Gtk::Button* pButton = NULL;
    
    // connect signals
    refXml->get_widget("cancel_button", pButton);
    if(pButton != NULL)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &TemplateMatchingParamsWin::on_cancel_button_clicked));
    
    refXml->get_widget("ok_button", pButton);
    if(pButton != NULL)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &TemplateMatchingParamsWin::on_ok_button_clicked) );
  

    refXml->get_widget("hscale_threshold_hc", hscale_threshold_hc);
    if(hscale_threshold_hc != NULL) {
      hscale_threshold_hc->set_value(threshold_hc);
    }

    refXml->get_widget("hscale_threshold_detection", hscale_threshold_detection);
    if(hscale_threshold_detection != NULL) {
      hscale_threshold_detection->set_value(threshold_detection);
    }

    refXml->get_widget("entry_step_size_search", entry_step_size_search);
    if(entry_step_size_search != NULL) {
      char txt[100];
      snprintf(txt, sizeof(txt), "%d", max_step_size_search);
      entry_step_size_search->set_text(strdup(txt));
    }

    refXml->get_widget("combobox_scale_down", combobox_scale_down);
    if(combobox_scale_down != NULL) {
      unsigned int i;
      m_refTreeModel = Gtk::ListStore::create(m_Columns);
      combobox_scale_down->set_model(m_refTreeModel);
      int row_number = 0;
      for(i = 1; i <= scalmgr_get_max_zoom_out_factor(scaling_manager); i*=2, row_number++) {
	Gtk::TreeModel::Row row = *(m_refTreeModel->append());
	row[m_Columns.m_col_scaling] = i;
	if(i == preselected_scale_down) combobox_scale_down->set_active(row_number);

      }
      combobox_scale_down->pack_start(m_Columns.m_col_scaling);
      

    }

  }
}

TemplateMatchingParamsWin::~TemplateMatchingParamsWin() {
  delete pDialog;
}

ret_t TemplateMatchingParamsWin::run(double * threshold_hc,
				     double * threshold_detection,
				     unsigned int * max_step_size_search,
				     unsigned int * scale_down) {
  assert(threshold_hc != NULL);
  assert(threshold_detection != NULL);
  assert(max_step_size_search != NULL);
  assert(scale_down != NULL);

  *max_step_size_search = 0;
  *scale_down = 0;

  while(*max_step_size_search == 0 || *scale_down == 0) {
    pDialog->run();
    if(ok_clicked) {

      *max_step_size_search = atoi(entry_step_size_search->get_text().c_str());

      *threshold_hc = hscale_threshold_hc->get_value();
      *threshold_detection = hscale_threshold_detection->get_value();

      Gtk::TreeModel::iterator iter = combobox_scale_down->get_active();
      if(iter) {
	Gtk::TreeModel::Row row = *iter;
	if(row) {
	  *scale_down = row[m_Columns.m_col_scaling];
	}
      }

      if(*max_step_size_search > 0 && *scale_down > 0) {
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

void TemplateMatchingParamsWin::on_combo_changed() {

}
