#include "mactools.h"
#include "ov_client_orlandoviols.h"
#include "ov_render_tascar.h"
#include <stdint.h>
#include <string>

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
  // std::string lobby("http://box.orlandoviols.com/");
  std::string lobby("http://localhost:8083/");
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
