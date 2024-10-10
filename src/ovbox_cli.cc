/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2020-2024 Giso Grimm
 */
/*
 * ovbox_cli is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * ovbox_cli is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with ovbox_cli. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ov_client_orlandoviols.h"
#include "ov_render_tascar.h"
#include <fstream>
#include <libgen.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>

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

int main(int argc, char** argv)
{
  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);

  std::cout
      << "ovbox_cli is free software: you can redistribute it and/or modify\n"
         "it under the terms of the GNU General Public License as published\n"
         "by the Free Software Foundation, version 3 of the License.\n"
         "\n"
         "ovbox_cli is distributed in the hope that it will be useful,\n"
         "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
         "MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
         "GNU General Public License, version 3 for more details.\n"
         "\n"
         "You should have received a copy of the GNU General Public License,\n"
         "Version 3 along with ovbox_cli. If not, see "
         "<http://www.gnu.org/licenses/>.\n"
         "\n"
         "Copyright (c) 2020-2024 Giso Grimm\n\nversion: "
      << get_libov_version() << "\n";

#ifndef WIN32
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
#endif

  try {
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
    std::string protocol(js_cfg.value("protocol", "ov"));
    // std::string deviceid(js_cfg["deviceid"].as<std::string>(getmacaddr()));
    bool showdevname(false);
    int pinglogport(0);
    bool allowsystemmods(false);
    bool secondary(false);
    std::string zitapath("");
    const char* options = "s:hqvd:p:nf:z:a2";
    struct option long_options[] = {
        {"server", 1, 0, 's'},    {"help", 0, 0, 'h'},
        {"quiet", 0, 0, 'q'},     {"deviceid", 1, 0, 'd'},
        {"verbose", 0, 0, 'v'},   {"pinglogport", 1, 0, 'p'},
        {"devname", 0, 0, 'n'},   {"frontend", 1, 0, 'f'},
        {"zitapath", 1, 0, 'z'},  {"allowsystemmods", 0, 0, 'a'},
        {"secondary", 0, 0, '2'}, {0, 0, 0, 0}};
    int opt(0);
    int option_index(0);
    while((opt = getopt_long(argc, argv, options, long_options,
                             &option_index)) != -1) {
      switch(opt) {
      case 'h':
        app_usage("ovbox_cli", long_options, "");
        return 0;
      case 'q':
        verbose = 0;
        break;
      case 's':
        lobby = optarg;
        break;
      case 'd':
        deviceid = optarg;
        break;
      case 'p':
        pinglogport = atoi(optarg);
        break;
      case 'a':
        allowsystemmods = true;
        break;
      case 'v':
        verbose++;
        break;
      case 'n':
        showdevname = true;
        break;
      case 'f':
        protocol = optarg;
        break;
      case 'z':
        zitapath = optarg;
        break;
      case '2':
        secondary = true;
        break;
      }
    }
    if(secondary)
      deviceid += "_2";
    frontend_t frontend(FRONTEND_OV);
    if(protocol == "ov")
      frontend = FRONTEND_OV;
    else if(protocol == "ds")
      frontend = FRONTEND_DS;
    else
      throw ErrMsg("Invalid front end protocol \"" + protocol + "\".");
    if(showdevname) {
      std::string devname(getmacaddr());
      if(devname.size() > 6)
        devname.erase(0, 6);
      devname = "_" + devname;
      std::cout << devname << std::endl;
      return 0;
    }
    if(deviceid.empty()) {
      throw ErrMsg("Invalid (empty) device id. Please ensure that the network "
                   "device is active or specify a valid device id.");
    }
    if(verbose)
      std::cout << "creating renderer with device id \"" << deviceid
                << "\" and pinglogport " << pinglogport << ".\n";
    ov_render_tascar_t render(deviceid, pinglogport, secondary);
    if(verbose)
      render.set_seqerr_callback(log_seq_error, nullptr);
    if(verbose)
      std::cout << "creating frontend interface for " << lobby
                << " using protocol \"" << protocol << "\"." << std::endl;
    if(zitapath.size())
      render.set_zita_path(zitapath);
    if(allowsystemmods)
      render.set_allow_systemmods(true);
    ov_client_base_t* ovclient(NULL);
    switch(frontend) {
    case FRONTEND_OV:
      ovclient = new ov_client_orlandoviols_t(render, lobby);
      break;
    case FRONTEND_DS:
      throw ErrMsg("frontend protocol \"ds\" is not yet implemented");
      break;
    }
    if(verbose)
      std::cout << "starting services\n";
    ovclient->start_service();
    while(!quit_app) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if(ovclient->is_going_to_stop()) {
        quit_app = true;
      }
    }
    if(verbose)
      std::cout << "stopping services\n";
    ovclient->stop_service();
    delete ovclient;
  }
  catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
