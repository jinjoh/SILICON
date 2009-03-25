#include "ConnectionInspectorWin.h"
#include "lib/grid.h"

#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <set>

#define TM "ConnectionInspectorWin.cc"

ConnectionInspectorWin::ConnectionInspectorWin(Gtk::Window *parent, logic_model_t * lmodel) {

  this->lmodel = lmodel;
  this->parent = parent;

  char file[PATH_MAX];
  snprintf(file, PATH_MAX, "%s/glade/connection_inspector.glade", getenv("DEGATE_HOME"));

  assert(lmodel);

  set_opacity(0.5);


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
    refXml->get_widget("connection_inspector_dialog", pDialog);
    assert(pDialog);
    if(pDialog) {

  pDialog->set_opacity(0.5);

      // connect signals
      refXml->get_widget("close_button", pCloseButton);
      if(pCloseButton)
 	pCloseButton->signal_clicked().connect(sigc::mem_fun(*this, &ConnectionInspectorWin::on_close_button_clicked));

      refXml->get_widget("goto_button", pGotoButton);
      if(pGotoButton)
	pGotoButton->signal_clicked().connect(sigc::mem_fun(*this, &ConnectionInspectorWin::on_goto_button_clicked) );

      refXml->get_widget("current_object_label", current_object_label);
      refXml->get_widget("current_object_type_label", current_object_type_label);

      refListStore = Gtk::ListStore::create(m_Columns);

      refXml->get_widget("treeview", pTreeView);
      if(pTreeView) {
	pTreeView->set_model(refListStore);
	//pTreeView->append_column("ID", m_Columns.m_col_id);
	//pTreeView->append_column("Port ID", m_Columns.m_col_sub_id);
	pTreeView->append_column("Object type", m_Columns.m_col_object_type_name);
	//pTreeView->append_column("Parent", m_Columns.m_col_parent);
	pTreeView->append_column("Name", m_Columns.m_col_name);

	Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = pTreeView->get_selection();
	refTreeSelection->signal_changed().connect(sigc::mem_fun(*this, &ConnectionInspectorWin::on_selection_changed));
      }

    }

    pDialog->set_transient_for(*parent);
    disable_inspection();
}

ConnectionInspectorWin::~ConnectionInspectorWin() {
  delete pDialog;
}

void ConnectionInspectorWin::show() {
  pDialog->show();
}


void ConnectionInspectorWin::on_selection_changed() {
  debug(TM, "sth. selected");
  pGotoButton->set_sensitive(true);
}

void ConnectionInspectorWin::show_connections(object_ptr_t * curr_obj, 
					      lmodel_connection_t * connections) {

  char str[100];
  lmodel_wire_t * wire;
  lmodel_via_t * via;
  lmodel_gate_port_t * gate_port;
  Gtk::TreeModel::Row row;
  
  //lmodel_connection_t * conn_list = lmodel_get_all_connected_objects(connections);
  lmodel_connection_t * conn = connections; 

  while(conn != NULL) {

    if(conn->obj_ptr != curr_obj) {
      switch(conn->object_type) {
      case LM_TYPE_WIRE:
	wire = (lmodel_wire_t *)conn->obj_ptr;
	row = *(refListStore->append()); 
	row[m_Columns.m_col_id] = wire->id;
	row[m_Columns.m_col_sub_id] = 0;
	row[m_Columns.m_col_object_type_name] = "wire";
	row[m_Columns.m_col_parent] = "-";
	if(RET_IS_OK(lmodel_get_printable_string_for_obj(LM_TYPE_WIRE, wire, str, sizeof(str))))
	  row[m_Columns.m_col_name] = str;
	
	row[m_Columns.m_col_object_type] = LM_TYPE_WIRE;
	row[m_Columns.m_col_object_ptr] = (object_ptr_t *)wire;
	
	break;
      case LM_TYPE_VIA:
	via = (lmodel_via_t *)conn->obj_ptr;
	row = *(refListStore->append()); 
	row[m_Columns.m_col_id] = via->id;
	row[m_Columns.m_col_sub_id] = 0;
	row[m_Columns.m_col_object_type_name] = "via";
	row[m_Columns.m_col_parent] = "-";
	if(RET_IS_OK(lmodel_get_printable_string_for_obj(LM_TYPE_VIA, via, str, sizeof(str))))
	  row[m_Columns.m_col_name] = str;
	
	row[m_Columns.m_col_object_type] = LM_TYPE_VIA;
	row[m_Columns.m_col_object_ptr] = (object_ptr_t *)via;
	
	break;
      case LM_TYPE_GATE_PORT:
	
	gate_port = (lmodel_gate_port_t *)conn->obj_ptr;
	row = *(refListStore->append());
	row[m_Columns.m_col_id] = gate_port->gate->id;
	row[m_Columns.m_col_sub_id] = gate_port->port_id;
	row[m_Columns.m_col_object_type_name] = "gate port";
	
	if(RET_IS_OK(lmodel_get_printable_string_for_obj(LM_TYPE_GATE, gate_port->gate, str, sizeof(str))))
	  row[m_Columns.m_col_parent] = str;
	
	if(RET_IS_OK(lmodel_get_printable_string_for_obj(LM_TYPE_GATE_PORT, gate_port, str, sizeof(str))))
	  row[m_Columns.m_col_name] = str;
	
	row[m_Columns.m_col_object_type] = LM_TYPE_GATE_PORT;
	row[m_Columns.m_col_object_ptr] = (object_ptr_t *)gate_port;
	
	break;
      default:
	break;
      }
    }

    conn = conn->next;
  }

  /*
  if(conn_list != NULL) {
    ret_t r = lmodel_destroy_connections(conn_list);
    assert(RET_IS_OK(r));
    }*/

}

void ConnectionInspectorWin::set_gate_port(lmodel_gate_port_t * gate_port) {
  assert(gate_port != NULL);
  if(gate_port != NULL) {
    char str[100];
    if(RET_IS_OK(lmodel_get_printable_string_for_obj(LM_TYPE_GATE_PORT, gate_port, str, sizeof(str)))) {
      current_object_label->set_text(str);
    }
    current_object_type_label->set_text("Gate port");
    clear_list();
    show_connections((object_ptr_t *)gate_port, 
		     lmodel_get_connections_from_object(LM_TYPE_GATE_PORT, gate_port));
  }
}

void ConnectionInspectorWin::set_wire(lmodel_wire_t * wire) {
  assert(wire != NULL);
  if(wire != NULL) {
    char str[100];
    if(RET_IS_OK(lmodel_get_printable_string_for_obj(LM_TYPE_WIRE, wire, str, sizeof(str)))) {
      current_object_label->set_text(str);
    }
    current_object_type_label->set_text("Wire");
    clear_list();
    show_connections((object_ptr_t *)wire, 
		     lmodel_get_connections_from_object(LM_TYPE_WIRE, wire));
  }
}

void ConnectionInspectorWin::set_via(lmodel_via_t * via) {
  assert(via != NULL);
  if(via != NULL) {
    char str[100];
    if(RET_IS_OK(lmodel_get_printable_string_for_obj(LM_TYPE_VIA, via, str, sizeof(str)))) {
      current_object_label->set_text(str);
    }
    current_object_type_label->set_text("Via");
    clear_list();
    show_connections((object_ptr_t *)via, 
		     lmodel_get_connections_from_object(LM_TYPE_VIA, via));
  }
}

void ConnectionInspectorWin::clear_list() {
  refListStore->clear();
  pGotoButton->set_sensitive(false);
}

void ConnectionInspectorWin::disable_inspection() {
  current_object_label->set_text("---");
  current_object_type_label->set_text("---");
  pGotoButton->set_sensitive(false);
  clear_list();
}

void ConnectionInspectorWin::on_close_button_clicked() {
  pDialog->hide();
}

void ConnectionInspectorWin::on_goto_button_clicked() {
  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =  pTreeView->get_selection();
  if(refTreeSelection) {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(*iter) {
      Gtk::TreeModel::Row row = *iter;
      LM_OBJECT_TYPE object_type = row[m_Columns.m_col_object_type];
      object_ptr_t * object_ptr = row[m_Columns.m_col_object_ptr];

      signal_goto_button_clicked_(object_type, object_ptr);
    }
  }
}

sigc::signal<void, LM_OBJECT_TYPE, object_ptr_t * >& ConnectionInspectorWin::signal_goto_button_clicked() {
  return signal_goto_button_clicked_;
}
