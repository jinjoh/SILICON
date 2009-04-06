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

#include "SetNameWin.h"

#include <assert.h>
#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include <stdlib.h>

SetNameWin::SetNameWin(Gtk::Window *parent, Glib::ustring name) {

  assert(parent);
  this->parent = parent;
  ok_clicked = false;
  orig_name = name;

  char file[PATH_MAX];
  snprintf(file, PATH_MAX, "%s/glade/set_name.glade", getenv("DEGATE_HOME"));

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
  refXml->get_widget("set_name_dialog", pDialog);
  if(pDialog) {
    //Get the Glade-instantiated Button, and connect a signal handler:
    Gtk::Button* pButton = NULL;
    
    // connect signals
    refXml->get_widget("cancel_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &SetNameWin::on_cancel_button_clicked));
    
    refXml->get_widget("ok_button", pButton);
    if(pButton) {
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &SetNameWin::on_ok_button_clicked) );
      pButton->grab_focus();
    }

    refXml->get_widget("entry_name", entry);
    if(entry) {
      entry->set_text(name);
    }

  }
}

SetNameWin::~SetNameWin() {
  delete pDialog;
}


Glib::ustring SetNameWin::run() {
  pDialog->run();
  if(ok_clicked) return entry->get_text();
  else return orig_name;
}

void SetNameWin::on_ok_button_clicked() {
  ok_clicked = true;
  pDialog->hide();
}

void SetNameWin::on_cancel_button_clicked() {
  ok_clicked = false;
  pDialog->hide();
}

