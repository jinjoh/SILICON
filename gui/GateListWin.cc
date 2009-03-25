#include "GateListWin.h"
#include "GateConfigWin.h"
#include "lib/grid.h"

#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include <assert.h>
#include <stdlib.h>

GateListWin::GateListWin(Gtk::Window *parent, logic_model_t * lmodel) {

  this->lmodel = lmodel;
  this->parent = parent;

  char file[PATH_MAX];
  snprintf(file, PATH_MAX, "%s/glade/gate_list.glade", getenv("DEGATE_HOME"));

  assert(lmodel);
  //assert(lmodel->gate_set);

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
    refXml->get_widget("gate_list_dialog", pDialog);
    if(pDialog) {
      //Get the Glade-instantiated Button, and connect a signal handler:
      Gtk::Button* pButton = NULL;

      // connect signals
      refXml->get_widget("close_button", pButton);
      if(pButton)
 	pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateListWin::on_close_button_clicked));

      refXml->get_widget("add_button", pButton);
      if(pButton)
	pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateListWin::on_add_button_clicked) );

      refXml->get_widget("remove_button", pButton);
      if(pButton)
	pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateListWin::on_remove_button_clicked) );

      refXml->get_widget("edit_button", pButton);
      if(pButton)
	pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateListWin::on_edit_button_clicked) );

      refListStore = Gtk::ListStore::create(m_Columns);

      refXml->get_widget("treeview", pTreeView);
      if(pTreeView) {
	pTreeView->set_model(refListStore);
	pTreeView->append_column("ID", m_Columns.m_col_id);
	pTreeView->append_column("Short Name", m_Columns.m_col_short_name);
	pTreeView->append_column("Description", m_Columns.m_col_description);
      }

      lmodel_gate_template_set_t * ptr = lmodel->gate_template_set;
      while(ptr != NULL) {

	if(ptr->gate) {
	  Gtk::TreeModel::Row row = *(refListStore->append()); 

	  row[m_Columns.m_col_id] = ptr->gate->id;
	  if(ptr->gate->short_name) row[m_Columns.m_col_short_name] = ptr->gate->short_name;
	  if(ptr->gate->description) row[m_Columns.m_col_description] = ptr->gate->description;
	}

	ptr = ptr->next;
      }

    }
    else {
      std::cout << "Error: can't find gate_list_dialog" << std::endl;
    }

}

GateListWin::~GateListWin() {
  delete pDialog;
}

void GateListWin::run() {
  
  pDialog->run();
}

void GateListWin::on_close_button_clicked() {
  pDialog->hide();
}

void GateListWin::on_add_button_clicked() {

  lmodel_gate_template_t * tmpl = lmodel_create_gate_template();

  GateConfigWin gcWin(parent, lmodel, tmpl);
  if(gcWin.run() == true) {
    if(RET_IS_OK(lmodel_add_gate_template(lmodel, tmpl, 0))) {
      Gtk::TreeModel::Row row = *(refListStore->append()); 
      
      row[m_Columns.m_col_id] = tmpl->id;
      row[m_Columns.m_col_short_name] = tmpl->short_name;
      row[m_Columns.m_col_description] = tmpl->description;

    }
  }

  /*
  Gtk::TreeModel::Row row = *(refListStore->append()); 
  row[m_Columns.m_col_short_name] = "port name";
  row[m_Columns.m_col_description] = "fffffffoooooo";

  lmodel_add_gate_template_to_gate_set(gate_set, tmpl, 0);
  */

}

void GateListWin::on_remove_button_clicked() {
  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =  pTreeView->get_selection();
  if(refTreeSelection) {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(iter) {
      Gtk::TreeModel::Row row = *iter; 
      int obj_id = row[m_Columns.m_col_id];

      Gtk::MessageDialog dialog(*parent, "Are you sure you want to remove a gate template?", 
				true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
      dialog.set_title("Warning");      
      if(dialog.run() == Gtk::RESPONSE_YES) {
	
	lmodel_gate_template_t * tmpl = lmodel_get_gate_template_by_id(lmodel, obj_id);
	if(tmpl) {

	  if(RET_IS_NOT_OK(lmodel_remove_gate_template(lmodel, tmpl))) {
	    Gtk::MessageDialog dialog(*parent, "Can't remove template gate.", true, Gtk::MESSAGE_ERROR);
	    dialog.set_title("Error");
	    dialog.run();
	  }
	  else 
	    refListStore->erase(iter);
	}
	
      }
    }

  }
}

void GateListWin::on_edit_button_clicked() {

  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =  pTreeView->get_selection();
  if(refTreeSelection) {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(iter) {
      Gtk::TreeModel::Row row = *iter; 
      int obj_id = row[m_Columns.m_col_id];
      lmodel_gate_template_t * tmpl = lmodel_get_gate_template_by_id(lmodel, obj_id);

      GateConfigWin gcWin(parent, lmodel, tmpl);
      if(gcWin.run() == true) {
	row[m_Columns.m_col_id] = tmpl->id;
	row[m_Columns.m_col_short_name] = tmpl->short_name;
	row[m_Columns.m_col_description] = tmpl->description;
      }
    }
  }
}

