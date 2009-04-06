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

#include "GridConfigWin.h"
#include "lib/grid.h"

#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <set>

GridConfigWin::GridConfigWin(Gtk::Window *parent, grid_t * grid) {

  this->parent = parent;
  this->grid = grid;

  memcpy(&orig_grid, grid, sizeof(grid_t));

  char tmp[50];
  char file[PATH_MAX];
  snprintf(file, PATH_MAX, "%s/glade/grid_config.glade", getenv("DEGATE_HOME"));


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
    return ;
  }
#endif
  
  //Get the Glade-instantiated Dialog:
  refXml->get_widget("grid_config_dialog", pDialog);
  assert(pDialog);
  if(pDialog) {

    pDialog->set_transient_for(*parent);

    // connect signals
    refXml->get_widget("cancel_button", p_cancel_button);
    assert(p_cancel_button != NULL);
    if(p_cancel_button != NULL) 
      p_cancel_button->signal_clicked().connect(sigc::mem_fun(*this, &GridConfigWin::on_cancel_button_clicked));
    
    refXml->get_widget("ok_button", p_ok_button);
    assert(p_ok_button != NULL);
    if(p_ok_button != NULL) {
      p_ok_button->grab_focus();
      p_ok_button->signal_clicked().connect(sigc::mem_fun(*this, &GridConfigWin::on_ok_button_clicked) );
    }

    refXml->get_widget("horizontal_checkbutton", p_horizontal_checkbutton);
    assert(p_horizontal_checkbutton != NULL);
    if(p_horizontal_checkbutton != NULL) {
      p_horizontal_checkbutton->set_active(grid->horizontal_lines_enabled ? true : false);
      p_horizontal_checkbutton->signal_clicked().connect(sigc::mem_fun(*this, &GridConfigWin::on_horz_checkb_clicked) );
    }

    refXml->get_widget("vertical_checkbutton", p_vertical_checkbutton);
    assert(p_vertical_checkbutton != NULL);
    if(p_vertical_checkbutton != NULL) {
      p_vertical_checkbutton->set_active(grid->vertical_lines_enabled ? true : false);
      p_vertical_checkbutton->signal_clicked().connect(sigc::mem_fun(*this, &GridConfigWin::on_vert_checkb_clicked) );
    }

    refXml->get_widget("hscale_offset_x", p_scale_offset_x);
    assert(p_scale_offset_x != NULL);
    if(p_scale_offset_x != NULL) {
      p_adj_offset_x = new Gtk::Adjustment(grid->offset_x, 0.0, 512.0, 1);
      p_scale_offset_x->set_adjustment(*p_adj_offset_x);
      p_adj_offset_x->signal_value_changed().connect(sigc::mem_fun(*this, &GridConfigWin::on_offset_x_changed));
    }

    refXml->get_widget("hscale_offset_y", p_scale_offset_y);
    assert(p_scale_offset_y != NULL);
    if(p_scale_offset_y != NULL) {
      p_adj_offset_y = new Gtk::Adjustment(grid->offset_y, 0.0, 512.0, 1);
      p_scale_offset_y->set_adjustment(*p_adj_offset_y);
      p_adj_offset_y->signal_value_changed().connect(sigc::mem_fun(*this, &GridConfigWin::on_offset_y_changed));
    }

    refXml->get_widget("hscale_distance_x", p_scale_dist_x);
    assert(p_scale_dist_x != NULL);
    if(p_scale_dist_x != NULL) {
      p_adj_dist_x = new Gtk::Adjustment(grid->dist_x, 0.0, 512.0, 1);
      p_scale_dist_x->set_adjustment(*p_adj_dist_x);
      p_adj_dist_x->signal_value_changed().connect(sigc::mem_fun(*this, &GridConfigWin::on_dist_x_changed));
    }

    refXml->get_widget("hscale_distance_y", p_scale_dist_y);
    assert(p_scale_dist_y != NULL);
    if(p_scale_dist_y != NULL) {
      p_adj_dist_y = new Gtk::Adjustment(grid->dist_y, 0.0, 512.0, 1);
      p_scale_dist_y->set_adjustment(*p_adj_dist_y);
      p_adj_dist_y->signal_value_changed().connect(sigc::mem_fun(*this, &GridConfigWin::on_dist_y_changed));
    }

    refXml->get_widget("entry_distance_x", p_entry_dist_x);
    assert(p_entry_dist_x != NULL);
    if(p_entry_dist_x != NULL) {
      snprintf(tmp, sizeof(tmp), "%f", grid->dist_x);
      p_entry_dist_x->set_text(tmp);
      p_entry_dist_x->signal_changed().connect(sigc::mem_fun(*this, &GridConfigWin::on_entry_dist_x_changed));
    }

    refXml->get_widget("entry_distance_y", p_entry_dist_y);
    assert(p_entry_dist_y != NULL);
    if(p_entry_dist_y != NULL) {
      snprintf(tmp, sizeof(tmp), "%f", grid->dist_y);
      p_entry_dist_y->set_text(tmp);
      p_entry_dist_y->signal_changed().connect(sigc::mem_fun(*this, &GridConfigWin::on_entry_dist_y_changed));
    }

    on_horz_checkb_clicked();
    on_vert_checkb_clicked();
  }

}

GridConfigWin::~GridConfigWin() {

  delete p_adj_offset_x;
  delete p_adj_offset_y;
  delete p_adj_dist_x;
  delete p_adj_dist_y;

  delete pDialog;
}

void GridConfigWin::show() {
  pDialog->show();
}


void GridConfigWin::on_cancel_button_clicked() {
  pDialog->hide();
  memcpy(grid, &orig_grid, sizeof(grid_t));
  signal_changed_();

}

void GridConfigWin::on_ok_button_clicked() {
  pDialog->hide();
  signal_changed_();
}


void GridConfigWin::on_horz_checkb_clicked() {
  grid->horizontal_lines_enabled = p_horizontal_checkbutton->get_active() == true ? 1 : 0;

  if(grid->horizontal_lines_enabled) {
    p_scale_offset_y->set_sensitive(true);
    p_scale_dist_y->set_sensitive(true);
    p_entry_dist_y->set_sensitive(true);
  }
  else {
    p_scale_offset_y->set_sensitive(false);
    p_scale_dist_y->set_sensitive(false);
    p_entry_dist_y->set_sensitive(false);
  }
  signal_changed_();
}

void GridConfigWin::on_vert_checkb_clicked() {
  grid->vertical_lines_enabled = p_vertical_checkbutton->get_active() == true ? 1 : 0;

  if(grid->vertical_lines_enabled) {
    p_scale_offset_x->set_sensitive(true);
    p_scale_dist_x->set_sensitive(true);
    p_entry_dist_x->set_sensitive(true);
  }
  else {
    p_scale_offset_x->set_sensitive(false);
    p_scale_dist_x->set_sensitive(false);
    p_entry_dist_x->set_sensitive(false);
  }
  signal_changed_();
}


void GridConfigWin::on_offset_x_changed() {
  grid->offset_x = p_adj_offset_x->get_value();
  signal_changed_();
}

void GridConfigWin::on_offset_y_changed() {
  grid->offset_y = p_adj_offset_y->get_value();
  signal_changed_();
}

void GridConfigWin::on_dist_x_changed() {
  char tmp[50];
  grid->dist_x = p_adj_dist_x->get_value();
  snprintf(tmp, sizeof(tmp), "%f", grid->dist_x);
  p_entry_dist_x->set_text(tmp);
  signal_changed_();
}

void GridConfigWin::on_dist_y_changed() {
  char tmp[50];
  grid->dist_y = p_adj_dist_y->get_value();
  snprintf(tmp, sizeof(tmp), "%f", grid->dist_y);
  p_entry_dist_y->set_text(tmp);
  signal_changed_();
}


void GridConfigWin::on_entry_dist_x_changed() {
  double v = atof(p_entry_dist_x->get_text().c_str());
  if(v < 0 || v == HUGE_VAL) {
    p_entry_dist_x->set_text("0");
    v = 0;
  }

  grid->dist_x = v;
  p_adj_dist_x->set_value(v);
  signal_changed_();
}

void GridConfigWin::on_entry_dist_y_changed() {
  double v = atof(p_entry_dist_y->get_text().c_str());
  if(v < 0 || v == HUGE_VAL) {
    p_entry_dist_y->set_text("0");
    v = 0;
  }

  grid->dist_y = v;
  p_adj_dist_y->set_value(v);

  signal_changed_();
}



sigc::signal<void>& GridConfigWin::signal_changed() {
  return signal_changed_;
}
