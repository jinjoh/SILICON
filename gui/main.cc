#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <iostream>
#include <stdlib.h>

#include "MainWin.h"
#include "SplashWin.h"

int main(int argc, char ** argv) {

  if(getenv("DEGATE_HOME") == NULL ||
     getenv("DEGATE_PLUGINS") == NULL) {

    std::cout 
      << std::endl
      << "Error: Environment variable DEGATE_HOME and DEGATE_PLUGINS are undefined. Please set it, e.g. " 
      << std::endl
      << std::endl
      << "\texport DEGATE_HOME=/home/foo/degate"
      << std::endl
      << "\texport DEGATE_PLUGINS=/home/foo/degate/plugins"
      << std::endl
      << std::endl
      << std::endl
      << std::endl;
    exit(1);
  }

  if(!Glib::thread_supported()) Glib::thread_init();
  Gtk::Main kit(argc, argv);

  SplashWin * splashWin = new SplashWin(1500);
  Gtk::Main::run(*splashWin);
  setlocale(LC_ALL, "C");

  delete splashWin;

  MainWin mainWin;
  Gtk::Main::run(mainWin);
  return 1;
}
