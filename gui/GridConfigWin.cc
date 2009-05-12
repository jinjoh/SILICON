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

  char tmp[50];
  char file[PATH_MAX];


  this->parent = parent;
  this->grid = grid;

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
    
    refXml->get_widget("close_button", p_close_button);
    assert(p_close_button != NULL);
    if(p_close_button != NULL) {
      p_close_button->grab_focus();
      p_close_button->signal_clicked().connect(sigc::mem_fun(*this, &GridConfigWin::on_close_button_clicked) );
    }


    refXml->get_widget("regular_grid_radiobutton", p_regular_grid_rbutton);
    refXml->get_widget("unregular_grid_radiobutton", p_unregular_grid_rbutton);
    assert(p_regular_grid_rbutton != NULL && p_unregular_grid_rbutton != NULL);
    if(p_regular_grid_rbutton != NULL && p_unregular_grid_rbutton != NULL) {
      if(grid->grid_mode == USE_UNREGULAR_GRID) p_unregular_grid_rbutton->set_active(true);
      else p_regular_grid_rbutton->set_active(true);

      p_regular_grid_rbutton->signal_clicked().connect(sigc::mem_fun(*this, &GridConfigWin::on_rgrid_rbutton_clicked) );
      p_unregular_grid_rbutton->signal_clicked().connect(sigc::mem_fun(*this, &GridConfigWin::on_urgrid_rbutton_clicked) );
    }

    /*
     * regular grid
     */

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

    /*
     * unregular grid - horizontal lines
     */


    refXml->get_widget("uhg_checkbutton", p_uhg_checkbutton);
    assert(p_uhg_checkbutton != NULL);
    if(p_uhg_checkbutton != NULL) {
      p_uhg_checkbutton->set_active(grid->uhg_enabled ? true : false);
      p_uhg_checkbutton->signal_clicked().connect(sigc::mem_fun(*this, &GridConfigWin::on_uhg_checkb_clicked) );
    }

    ref_liststore_uhg = Gtk::ListStore::create(m_columns_uhg);
    refXml->get_widget("uhg_treeview", p_treeview_uhg);
    assert(p_treeview_uhg != NULL);
    if(p_treeview_uhg != NULL) {
      p_treeview_uhg->set_model(ref_liststore_uhg);
      Gtk::TreeView::Column * pColumn;

      Gtk::CellRendererText * pRenderer = Gtk::manage( new Gtk::CellRendererText()); 
      p_treeview_uhg->append_column("Offset", *pRenderer);
      pColumn = p_treeview_uhg->get_column(0);
      pColumn->add_attribute(*pRenderer, "text", m_columns_uhg.m_col_offset); 
      pRenderer->property_editable() = true;
      pRenderer->signal_edited().connect(sigc::mem_fun(*this, &GridConfigWin::on_uhg_edited));

      pColumn = p_treeview_uhg->get_column(0);
      if(pColumn) pColumn->set_sort_column(m_columns_uhg.m_col_offset);

      Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =  p_treeview_uhg->get_selection();
      refTreeSelection->set_mode(Gtk::SELECTION_MULTIPLE);
    }

    refXml->get_widget("button_add_uhg", p_button_add_uhg);
    assert(p_button_add_uhg != NULL);
    if(p_button_add_uhg != NULL) {
      p_button_add_uhg->signal_clicked().connect(sigc::mem_fun(*this, &GridConfigWin::on_button_add_uhg_clicked) );
    }

    refXml->get_widget("button_remove_uhg", p_button_remove_uhg);
    assert(p_button_remove_uhg != NULL);
    if(p_button_remove_uhg != NULL) {
      p_button_remove_uhg->signal_clicked().connect(sigc::mem_fun(*this, &GridConfigWin::on_button_remove_uhg_clicked) );
    }

    /*
     * unregular grid - horizontal lines
     */


    refXml->get_widget("uvg_checkbutton", p_uvg_checkbutton);
    assert(p_uvg_checkbutton != NULL);
    if(p_uvg_checkbutton != NULL) {
      p_uvg_checkbutton->set_active(grid->uvg_enabled ? true : false);
      p_uvg_checkbutton->signal_clicked().connect(sigc::mem_fun(*this, &GridConfigWin::on_uvg_checkb_clicked) );
    }

    ref_liststore_uvg = Gtk::ListStore::create(m_columns_uvg);
    refXml->get_widget("uvg_treeview", p_treeview_uvg);
    assert(p_treeview_uvg != NULL);
    if(p_treeview_uvg != NULL) {
      p_treeview_uvg->set_model(ref_liststore_uvg);
      Gtk::TreeView::Column * pColumn;

      Gtk::CellRendererText * pRenderer = Gtk::manage( new Gtk::CellRendererText()); 
      p_treeview_uvg->append_column("Offset", *pRenderer);
      pColumn = p_treeview_uvg->get_column(0);
      pColumn->add_attribute(*pRenderer, "text", m_columns_uvg.m_col_offset); 
      pRenderer->property_editable() = true;
      pRenderer->signal_edited().connect(sigc::mem_fun(*this, &GridConfigWin::on_uvg_edited));


      pColumn = p_treeview_uvg->get_column(0);
      if(pColumn) pColumn->set_sort_column(m_columns_uvg.m_col_offset);      

      Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =  p_treeview_uhg->get_selection();
      refTreeSelection->set_mode(Gtk::SELECTION_MULTIPLE);
    }

    refXml->get_widget("button_add_uvg", p_button_add_uvg);
    assert(p_button_add_uvg != NULL);
    if(p_button_add_uvg != NULL) {
      p_button_add_uvg->signal_clicked().connect(sigc::mem_fun(*this, &GridConfigWin::on_button_add_uvg_clicked) );
    }

    refXml->get_widget("button_remove_uvg", p_button_remove_uvg);
    assert(p_button_remove_uvg != NULL);
    if(p_button_remove_uvg != NULL) {
      p_button_remove_uvg->signal_clicked().connect(sigc::mem_fun(*this, &GridConfigWin::on_button_remove_uvg_clicked) );
    }

    update_grid_entries();
    on_uhg_checkb_clicked();
    on_uvg_checkb_clicked();

  }

}

GridConfigWin::~GridConfigWin() {

  delete p_adj_offset_x;
  delete p_adj_offset_y;
  delete p_adj_dist_x;
  delete p_adj_dist_y;

  delete pDialog;
}


void GridConfigWin::update_grid_entries() {
  unsigned int i;
  if(p_treeview_uvg != NULL) {
    ref_liststore_uvg->clear();
    for(i = 0; i < grid->num_uvg_entries; i++) {
      Gtk::TreeModel::Row row = *(ref_liststore_uvg->append()); 
      row[m_columns_uvg.m_col_offset] = grid->uvg_offsets[i];
    }
  }

  if(p_treeview_uhg != NULL) {
    ref_liststore_uhg->clear();
    for(i = 0; i < grid->num_uhg_entries; i++) {
      Gtk::TreeModel::Row row = *(ref_liststore_uhg->append()); 
      row[m_columns_uhg.m_col_offset] = grid->uhg_offsets[i];
    }
  }
}

void GridConfigWin::on_rgrid_rbutton_clicked() {
  grid->grid_mode = USE_REGULAR_GRID;
}

void GridConfigWin::on_urgrid_rbutton_clicked() {
  grid->grid_mode = USE_UNREGULAR_GRID;  
}


void GridConfigWin::on_uhg_edited(const Glib::ustring& path, const Glib::ustring& new_text) {
  //debug(TM, "edited signal path=%s new text=%s", path.c_str(), new_text.c_str());

  Gtk::TreeModel::iterator iter = ref_liststore_uhg->get_iter(path);
  if(iter) {
    Gtk::TreeModel::Row row = *iter;
    
    unsigned int new_offset = atol(new_text.c_str());
    row[m_columns_uhg.m_col_offset] = new_offset;

    update_uhg_structs();
  }

}

void GridConfigWin::on_uvg_edited(const Glib::ustring& path, const Glib::ustring& new_text) {
  //debug(TM, "edited signal path=%s new text=%s", path.c_str(), new_text.c_str());

  Gtk::TreeModel::iterator iter = ref_liststore_uvg->get_iter(path);
  if(iter) {
    Gtk::TreeModel::Row row = *iter;
    
    unsigned int new_offset = atol(new_text.c_str());
    row[m_columns_uvg.m_col_offset] = new_offset;

    update_uvg_structs();
  }

}

void GridConfigWin::show() {
  pDialog->show();
}

void GridConfigWin::on_close_button_clicked() {
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


void GridConfigWin::on_uhg_checkb_clicked() {
  grid->uhg_enabled = p_uhg_checkbutton->get_active() == true ? 1 : 0;

  if(grid->uhg_enabled == 1) {
    p_button_add_uhg->set_sensitive(true);
    p_button_remove_uhg->set_sensitive(true);
    p_treeview_uhg->set_sensitive(true);
  }
  else {
    p_button_add_uhg->set_sensitive(false);
    p_button_remove_uhg->set_sensitive(false);
    p_treeview_uhg->set_sensitive(false);
  }
  signal_changed_();

}

void GridConfigWin::on_uvg_checkb_clicked() {
  grid->uvg_enabled =  p_uvg_checkbutton->get_active() == true ? 1 : 0;

  if(grid->uvg_enabled == 1) {
    p_button_add_uvg->set_sensitive(true);
    p_button_remove_uvg->set_sensitive(true);
    p_treeview_uvg->set_sensitive(true);
  }
  else {
    p_button_add_uvg->set_sensitive(false);
    p_button_remove_uvg->set_sensitive(false);
    p_treeview_uvg->set_sensitive(false);
  }
  signal_changed_();

}


unsigned int * GridConfigWin::model_data_to_offset_array(Glib::RefPtr<Gtk::ListStore> &ref_liststore,
							 unsigned int * num_entries,
							 GridConfigModelColumns * p_m_cols) {
  typedef Gtk::TreeModel::Children type_children;

  type_children children = ref_liststore->children();

  *num_entries = children.size();

  unsigned int * ptr = (unsigned int *) malloc(*num_entries * sizeof(unsigned int));
  assert(ptr != NULL);
  if(ptr == NULL) return NULL;

  unsigned int i = 0;
  for(type_children::iterator iter = children.begin(); iter != children.end(); ++iter, i++) {
    Gtk::TreeModel::Row row = *iter;
    ptr[i] = row[p_m_cols->m_col_offset];
  }

  return ptr;
}

void GridConfigWin::update_uhg_structs() {
  unsigned int entries = 0;
  unsigned int * offsets = model_data_to_offset_array(ref_liststore_uhg, &entries, &m_columns_uhg);

  if(RET_IS_NOT_OK(grid_set_uhg_offsets(grid, offsets, entries))) {
    debug(TM, "setting grid entries failed.");
  }
  signal_changed_();

}

void GridConfigWin::update_uvg_structs() {
  unsigned int entries = 0;
  unsigned int * offsets = model_data_to_offset_array(ref_liststore_uvg, &entries, &m_columns_uvg);

  if(RET_IS_NOT_OK(grid_set_uvg_offsets(grid, offsets, entries))) {
    debug(TM, "setting grid entries failed.");
  }
  signal_changed_();

}

void GridConfigWin::on_button_add_uhg_clicked() {
  Gtk::TreeModel::Row row = *(ref_liststore_uhg->append()); 
  row[m_columns_uhg.m_col_offset] = 0;
  //update_uhg_structs();
}

void GridConfigWin::on_button_add_uvg_clicked() {
  Gtk::TreeModel::Row row = *(ref_liststore_uvg->append()); 
  row[m_columns_uvg.m_col_offset] = 0;
  //update_uvg_structs();
}


void GridConfigWin::on_button_remove_uhg_clicked() {
  Glib::RefPtr<Gtk::TreeSelection> ref_tree_selection = p_treeview_uhg->get_selection();
  if(ref_tree_selection) {

    std::vector<Gtk::TreeModel::Path> pathlist = ref_tree_selection->get_selected_rows();

    for(std::vector<Gtk::TreeModel::Path>::reverse_iterator iter = pathlist.rbegin(); 
	iter != pathlist.rend(); ++iter)
      ref_liststore_uhg->erase(ref_tree_selection->get_model()->get_iter (*iter));
    
    update_uhg_structs();
  }
}

void GridConfigWin::on_button_remove_uvg_clicked() {
  Glib::RefPtr<Gtk::TreeSelection> ref_tree_selection =  p_treeview_uvg->get_selection();
  if(ref_tree_selection) {

    std::vector<Gtk::TreeModel::Path> pathlist = ref_tree_selection->get_selected_rows();

    for(std::vector<Gtk::TreeModel::Path>::reverse_iterator iter = pathlist.rbegin(); 
	iter != pathlist.rend(); ++iter)
      ref_liststore_uvg->erase(ref_tree_selection->get_model()->get_iter (*iter));
    
    update_uvg_structs();
  }
}
