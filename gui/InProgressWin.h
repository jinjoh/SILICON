#ifndef __INPROGRESSWIN_H__
#define __INPROGRESSWIN_H__

#include <gtkmm.h>

class InProgressWin : public Gtk::Window {
 public:
  InProgressWin(Gtk::Window *parent, const Glib::ustring& title, const Glib::ustring& message);
  virtual ~InProgressWin();
  void close();

 private:

  bool running;
#ifdef IMPL_WITH_THREAD
  Glib::Thread * thread;
  Glib::Dispatcher   signal_progress_;
  void progress_thread();
#endif

  Gtk::VBox m_Box;
  Gtk::Label m_Label_Message;
  Gtk::ProgressBar m_ProgressBar;

  bool update_progress_bar();
};

#endif
