#include "ov_client_orlandoviols.h"
#include "ov_render_tascar.h"
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
  std::string deviceid(getmacaddr());
  std::string lobby("https://oldbox.orlandoviols.com/");
  // std::string lobby("http://localhost:8083/");
  const char* options = "s:hqvd:";
  struct option long_options[] = {
      {"server", 1, 0, 's'},   {"help", 0, 0, 'h'},    {"quiet", 0, 0, 'q'},
      {"deviceid", 1, 0, 'd'}, {"verbose", 0, 0, 'v'}, {0, 0, 0, 0}};
  int opt(0);
  int option_index(0);
  while((opt = getopt_long(argc, argv, options, long_options, &option_index)) !=
        -1) {
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
    case 'v':
      verbose++;
      break;
    }
  }
  ov_render_tascar_t render(deviceid);
  ov_client_orlandoviols_t ovclient(render, lobby);
  ovclient.start_service();
  while(!quit_app)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ovclient.stop_service();
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
