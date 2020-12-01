#include "ov_client_digitalstage.h"
#include <iostream>

ov_client_digitalstage_t::ov_client_digitalstage_t(
    ov_render_base_t& backend, const std::string& frontend_url_)
    : ov_client_base_t(backend), runservice(true), frontend_url(frontend_url_),
      quitrequest_(false)
{
}

void ov_client_digitalstage_t::start_service()
{
  runservice = true;
  servicethread = std::thread(&ov_client_digitalstage_t::service, this);
}

void ov_client_digitalstage_t::stop_service()
{
  runservice = false;
  servicethread.join();
}

void ov_client_digitalstage_t::service()
{
  // register_device(lobby, backend.get_deviceid());
  // download_file(lobby + "/announce.flac", "announce.flac");
  // start main control loop:
  while(runservice) {
    std::cerr << "Error: not yet implemented." << std::endl;
    quitrequest_ = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
