/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2020 2022 Giso Grimm
 */
/*
 * ov-client is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * ov-client is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with ov-client. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ov_client_orlandoviols.h"
#include "ov_render_tascar.h"
#include <fstream>
#include <gtkmm.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>

#include "ovbox.res.c"

#include "ovbox_glade.h"

#define GET_WIDGET(x)                                                          \
  m_refBuilder->get_widget(#x, x);                                             \
  if(!x)                                                                       \
  throw TASCAR::ErrMsg(std::string("No widget \"") + #x +                      \
                       std::string("\" in builder."))

enum frontend_t { FRONTEND_OV, FRONTEND_DS };

static bool quit_app(false);

static void sighandler(int sig)
{
  quit_app = true;
}

std::string get_file_contents(const std::string& fname)
{
  std::ifstream t(fname);
  std::string str((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());
  return str;
}

void log_seq_error(stage_device_id_t cid, sequence_t seq_ex, sequence_t seq_rec,
                   port_t p, void*)
{
  std::cout << "sequence error, cid=" << (int)cid << " expected " << seq_ex
            << " received " << seq_rec << " diff " << seq_rec - seq_ex
            << " port " << p << std::endl;
}

class ovboxgui_t : public Gtk::Window {
public:
  ovboxgui_t(BaseObjectType* cobject,
             const Glib::RefPtr<Gtk::Builder>& refGlade);
  virtual ~ovboxgui_t();
  void update_display();

  std::string zitapath = "";
  std::string ui_url = "http://box.orlandoviols.com/";

protected:
  bool on_timeout();
  void on_uiurl_clicked();
  void on_mixer_clicked();
  void runclient();

  sigc::connection con_timeout;

  Glib::RefPtr<Gtk::Builder> m_refBuilder;
  // Signal handlers:
  // void on_quit();
  Gtk::Label* label;
  Gtk::Label* labdevice;
  Gtk::Label* labuser;
  Gtk::Button* buttonmixer;
  Gtk::Button* buttonopen;
  // calibsession_t* session;
  std::thread clientthread;
  std::mutex mcl;
  ov_client_base_t* ovclient = NULL;
  std::string errmsg;
  bool session_ready = false;
};

ovboxgui_t::ovboxgui_t(BaseObjectType* cobject,
                       const Glib::RefPtr<Gtk::Builder>& refGlade)
    : Gtk::Window(cobject), m_refBuilder(refGlade), label(NULL),
      buttonmixer(NULL), buttonopen(NULL)
{
  GET_WIDGET(label);
  GET_WIDGET(labuser);
  GET_WIDGET(labdevice);
  GET_WIDGET(buttonmixer);
  GET_WIDGET(buttonopen);
  update_display();
  clientthread = std::thread(&ovboxgui_t::runclient, this);
  con_timeout = Glib::signal_timeout().connect(
      sigc::mem_fun(*this, &ovboxgui_t::on_timeout), 250);
  buttonopen->signal_clicked().connect(
      sigc::mem_fun(*this, &ovboxgui_t::on_uiurl_clicked));
  buttonmixer->signal_clicked().connect(
      sigc::mem_fun(*this, &ovboxgui_t::on_mixer_clicked));
  show_all();
}

void ovboxgui_t::on_uiurl_clicked()
{
  gtk_show_uri(NULL, ui_url.c_str(), GDK_CURRENT_TIME, NULL);
  // gtk_show_uri_on_window( NULL, ui_url.c_str(), GDK_CURRENT_TIME, NULL );
}

void ovboxgui_t::on_mixer_clicked()
{
  std::string url("http://" + ep2ipstr(getipaddr()) + ":8080/");
  gtk_show_uri(NULL, url.c_str(), GDK_CURRENT_TIME, NULL);
  // gtk_show_uri_on_window( NULL, ui_url.c_str(), GDK_CURRENT_TIME, NULL );
}

bool ovboxgui_t::on_timeout()
{
  if(mcl.try_lock()) {
    if(ovclient) {
      std::string owner = ovclient->get_owner();
      labuser->set_label(owner);
      if(errmsg.empty()) {
        if(owner.empty()) {
          label->set_label(
              "Your device is not connected to an account. Please visit\n" +
              ui_url + "\nto connect your device.");
        } else {
          label->set_label("");
        }
      }
    }
    mcl.unlock();
  }
  return true;
}

void ovboxgui_t::runclient()
{
  while(!quit_app) {

    try {
      labdevice->get_style_context()->add_class("passmember");
      labdevice->get_style_context()->remove_class("actmember");
      session_ready = false;
      // test for config file on raspi:
      std::string config(get_file_contents("/boot/ov-client.cfg"));
      if(config.empty() && std::getenv("HOME"))
        // we are not on a raspi, or no file was created, thus check in home
        // directory
        config = get_file_contents(std::string(std::getenv("HOME")) +
                                   std::string("/.ov-client.cfg"));
      if(config.empty())
        // we are not on a raspi, or no file was created, thus check in local
        // directory
        config = get_file_contents("ov-client.cfg");
      nlohmann::json js_cfg({{"deviceid", getmacaddr()},
                             {"url", "https://oldbox.orlandoviols.com/"},
                             {"protocol", "ov"}});
      if(!config.empty()) {
        try {
          js_cfg = nlohmann::json::parse(config);
        }
        catch(const std::exception& err) {
          DEBUG(config);
          DEBUG(err.what());
        }
      }
      std::string deviceid(js_cfg.value("deviceid", getmacaddr()));
      std::string lobby(ovstrrep(
          js_cfg.value("url", "http://oldbox.orlandoviols.com/"), "\\/", "/"));
      ui_url = ovstrrep(js_cfg.value("ui", "http://box.orlandoviols.com/"),
                        "\\/", "/");
      std::string protocol(js_cfg.value("protocol", "ov"));
      int pinglogport(0);
      bool allowsystemmods(false);
      frontend_t frontend(FRONTEND_OV);
      if(protocol == "ov")
        frontend = FRONTEND_OV;
      else if(protocol == "ds")
        frontend = FRONTEND_DS;
      else
        throw ErrMsg("Invalid front end protocol \"" + protocol + "\".");
      if(deviceid.empty()) {
        throw ErrMsg(
            "Invalid (empty) device id. Please ensure that the network "
            "device is active or specify a valid device id.");
      }
      if(verbose)
        std::cout << "creating renderer with device id \"" << deviceid
                  << "\" and pinglogport " << pinglogport << ".\n";
      labdevice->set_label(deviceid);
      buttonopen->set_label(ui_url);
      ov_render_tascar_t render(deviceid, pinglogport);
      if(verbose)
        render.set_seqerr_callback(log_seq_error, nullptr);
      if(verbose)
        std::cout << "creating frontend interface for " << lobby
                  << " using protocol \"" << protocol << "\"." << std::endl;
      if(zitapath.size())
        render.set_zita_path(zitapath);
      if(allowsystemmods)
        render.set_allow_systemmods(true);
      switch(frontend) {
      case FRONTEND_OV:
        mcl.lock();
        ovclient = new ov_client_orlandoviols_t(render, lobby);
        mcl.unlock();
        break;
      case FRONTEND_DS:
        throw ErrMsg("frontend protocol \"ds\" is not yet implemented");
        break;
      }
      if(verbose)
        std::cout << "starting services\n";
      ovclient->start_service();
      errmsg = "";
      bool run_session(true);
      while((!quit_app) && run_session) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        bool now_ready = ovclient->is_session_ready();
        if(session_ready != now_ready) {
          session_ready = now_ready;
          if(session_ready) {
            labdevice->get_style_context()->add_class("actmember");
            labdevice->get_style_context()->remove_class("passmember");
          } else {
            labdevice->get_style_context()->add_class("passmember");
            labdevice->get_style_context()->remove_class("actmember");
          }
        }
        if(ovclient->is_going_to_stop()) {
          run_session = false;
        }
      }
      labdevice->get_style_context()->add_class("passmember");
      labdevice->get_style_context()->remove_class("actmember");
      if(verbose)
        std::cout << "stopping services\n";
      ovclient->stop_service();
      mcl.lock();
      delete ovclient;
      ovclient = NULL;
      mcl.unlock();
    }
    catch(const std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      errmsg = e.what();
      label->set_label(errmsg);
    }
    if(!quit_app)
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
  hide();
}

ovboxgui_t::~ovboxgui_t()
{
  con_timeout.disconnect();
  quit_app = true;
  if(clientthread.joinable())
    clientthread.join();
}

void ovboxgui_t::update_display() {}

int main(int argc, char** argv)
{
  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);

  std::cout
      << "ov-client is free software: you can redistribute it and/or modify\n"
         "it under the terms of the GNU General Public License as published\n"
         "by the Free Software Foundation, version 3 of the License.\n"
         "\n"
         "ov-client is distributed in the hope that it will be useful,\n"
         "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
         "MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
         "GNU General Public License, version 3 for more details.\n"
         "\n"
         "You should have received a copy of the GNU General Public License,\n"
         "Version 3 along with ov-client. If not, see "
         "<http://www.gnu.org/licenses/>.\n"
         "\n"
         "Copyright (c) 2020-2022 Giso Grimm\n\nversion: "
      << get_libov_version() << "\n";

  // update search path to contain directory of this binary:
  char* rpath = realpath(argv[0], NULL);
  std::string rdir = dirname(rpath);
  free(rpath);
  char* epath = getenv("PATH");
  std::string epaths;
  if(epath)
    epaths = epath;
  if(epaths.size())
    epaths += ":";
  epaths += rdir;
  setenv("PATH", epaths.c_str(), 1);

  const char* options = "hvz:";
  struct option long_options[] = {{"help", 0, 0, 'h'},
                                  {"verbose", 0, 0, 'v'},
                                  {"zitapath", 1, 0, 'z'},
                                  {0, 0, 0, 0}};
  int opt(0);
  int option_index(0);
  std::string zitapath;
  verbose = 0;
  while((opt = getopt_long(argc, argv, options, long_options, &option_index)) !=
        -1) {
    switch(opt) {
    case 'h':
      app_usage("ov-client", long_options, "");
      return 0;
    case 'v':
      verbose++;
      break;
    case 'z':
      zitapath = optarg;
      break;
    }
  }
  auto nargc = argc;
  Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(
      nargc, argv, "com.orlandoviols.ovbox", Gio::APPLICATION_NON_UNIQUE);
  Glib::RefPtr<Gtk::Builder> refBuilder;
  ovboxgui_t* win(NULL);
  refBuilder = Gtk::Builder::create();
  refBuilder->add_from_string(ui_ovbox);
  refBuilder->get_widget_derived("mainwin", win);
  if(!win)
    throw TASCAR::ErrMsg("No main window");
  if(zitapath.size())
    win->zitapath = zitapath;

  auto css = Gtk::CssProvider::create();
  if(css->load_from_data(
         ".ovbox { background-color: #4e6263;font-weight: bold; } "
         ".status { color: #000; margin: 2px; } "
         ".warn { color: #ff9999; margin: 2px; } "
         ".mixerurl { color: #0000ff; background-color: #919a91; "
         "border-radius: 7px; margin: 4px; background-image: none; "
         "border-image: none; } "
         ".input { color: #0000ff; background-color: #919a91; "
         "border-radius: 7px; margin: 4px; background-image: none; "
         "border-image: none; } "
         ".actmember { margin-left: 4px; margin-right: 4px; "
         "margin-top: 2px; margin-bottom: 4px; background-color: #ecc348; "
         "padding: "
         "5px; border-radius: 9px; color: #000000; "
         "}"
         ".passmember { margin-left: 4px; margin-right: 4px; "
         "background-color: #d6d6d6;"
         "color: #666666;"
         "padding: 5px;"
         "border-radius: 7px;"
         "margin-top: 2px; margin-bottom: 4px; "
         "}")) {
    auto screen = Gdk::Screen::get_default();
    auto ctx = win->get_style_context();
    ctx->add_provider_for_screen(screen, css,
                                 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }
  win->show_all();
  int rv(app->run(*win));
  delete win;
  return rv;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
