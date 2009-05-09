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

#include "GenericTextInputWin.h"

#include <assert.h>
#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include <stdlib.h>

GenericTextInputWin::GenericTextInputWin(Gtk::Window *parent,
					 Glib::ustring title,
					 Glib::ustring label_text, 
					 Glib::ustring preset_text) {
  
  assert(parent);
  this->parent = parent;
  ok_clicked = false;
  orig_text = preset_text;

  char file[PATH_MAX];
  snprintf(file, PATH_MAX, "%s/glade/generic_text_input.glade", getenv("DEGATE_HOME"));

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
  refXml->get_widget("generic_text_input_dialog", pDialog);
  if(pDialog) {
    //Get the Glade-instantiated Button, and connect a signal handler:
    Gtk::Button* pButton = NULL;
    
    pDialog->set_title(title);

    // connect signals
    refXml->get_widget("cancel_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &GenericTextInputWin::on_cancel_button_clicked));
    
    refXml->get_widget("ok_button", pButton);
    if(pButton) {
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &GenericTextInputWin::on_ok_button_clicked) );
      pButton->grab_focus();
    }

    refXml->get_widget("label", label);
    if(label != NULL) {
      label->set_text(label_text);
    }

    refXml->get_widget("entry", entry);
    if(entry != NULL) {
      entry->grab_focus();
      entry->set_text(preset_text);
    }

  }
}

GenericTextInputWin::~GenericTextInputWin() {
  delete pDialog;
}


Glib::ustring GenericTextInputWin::run() {
  pDialog->run();
  if(ok_clicked) return entry->get_text();
  else return orig_text;
}

void GenericTextInputWin::on_ok_button_clicked() {
  ok_clicked = true;
  pDialog->hide();
}

void GenericTextInputWin::on_cancel_button_clicked() {
  ok_clicked = false;
  pDialog->hide();
}

