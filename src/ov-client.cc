#include "ov_client_orlandoviols.h"
#include "ov_render_tascar.h"
#include <errmsg.h>
#include <stdint.h>
#include <string>
#include <udpsocket.h>

static bool quit_app(false);

static void sighandler(int sig)
{
  quit_app = true;
}

int main(int argc, char** argv)
{
  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);
  try {
    std::string deviceid(getmacaddr());
    std::string lobby("http://oldbox.orlandoviols.com/");
    int pinglogport(0);
    const char* options = "s:hqvd:p:";
    struct option long_options[] = {{"server", 1, 0, 's'},
                                    {"help", 0, 0, 'h'},
                                    {"quiet", 0, 0, 'q'},
                                    {"deviceid", 1, 0, 'd'},
                                    {"verbose", 0, 0, 'v'},
                                    {"pinglogport", 1, 0, 'p'},
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
      }
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
      std::cout << "creating frontend interface for " << lobby << std::endl;
    ov_client_orlandoviols_t ovclient(render, lobby);
    if(verbose)
      std::cout << "starting services\n";
    ovclient.start_service();
    while(!quit_app) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if(ovclient.quitrequest()) {
        quit_app = true;
      }
    }
    if(verbose)
      std::cout << "stopping services\n";
    ovclient.stop_service();
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
