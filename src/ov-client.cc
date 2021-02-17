//#include "ov_client_digitalstage.h"
#include "ov_client_orlandoviols.h"
#include "ov_render_tascar.h"
#include <errmsg.h>
#include <fstream>
#include <stdint.h>

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

int main(int argc, char** argv)
{
  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);

  // boost::filesystem::path selfpath = argv[0];

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
                           {"url", "http://oldbox.orlandoviols.com/"},
                           {"protocol", "ov"}});
    if(!config.empty())
      js_cfg = nlohmann::json::parse(config);
    std::string deviceid(js_cfg.value("deviceid", getmacaddr()));
    std::string lobby(ovstrrep(
        js_cfg.value("url", "http://oldbox.orlandoviols.com/"), "\\/", "/"));
    std::string protocol(js_cfg.value("protocol", "ov"));
    // std::string deviceid(js_cfg["deviceid"].as<std::string>(getmacaddr()));
    bool showdevname(false);
    int pinglogport(0);
    const char* options = "s:hqvd:p:nf:";
    struct option long_options[] = {{"server", 1, 0, 's'},
                                    {"help", 0, 0, 'h'},
                                    {"quiet", 0, 0, 'q'},
                                    {"deviceid", 1, 0, 'd'},
                                    {"verbose", 0, 0, 'v'},
                                    {"pinglogport", 1, 0, 'p'},
                                    {"devname", 0, 0, 'n'},
                                    {"frontend", 1, 0, 'f'},
                                    {0, 0, 0, 0}};
    int opt(0);
    int option_index(0);
    while((opt = getopt_long(argc, argv, options, long_options,
                             &option_index)) != -1) {
      switch(opt) {
      case 'h':
        app_usage("ov-client", long_options, "");
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
      case 'v':
        verbose++;
        break;
      case 'n':
        showdevname = true;
        break;
      case 'f':
        protocol = optarg;
        break;
      }
    }
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
    ov_render_tascar_t render(deviceid, pinglogport);
    if(verbose)
      std::cout << "creating frontend interface for " << lobby
                << " using protocol \"" << protocol << "\"." << std::endl;
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
