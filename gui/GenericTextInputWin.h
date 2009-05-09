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

#ifndef __GENERICTEXTINPUTWIN_H__
#define __GENERICTEXTINPUTWIN_H__

#include <gtkmm.h>

#include "lib/project.h"
#include "lib/logic_model.h"

class GenericTextInputWin  {

 public:
  GenericTextInputWin(Gtk::Window *parent, 
		      Glib::ustring title,
		      Glib::ustring label_text, 
		      Glib::ustring preset_text);
  virtual ~GenericTextInputWin();
        
  Glib::ustring run();

 private:
  Gtk::Window *parent;
  Gtk::Dialog* pDialog;
  Gtk::Entry * entry;
  Gtk::Label * label;
  Glib::ustring orig_text;
  bool ok_clicked;

  // Signal handlers:
  virtual void on_ok_button_clicked();
  virtual void on_cancel_button_clicked();

};

#endif
