#ifndef OV_CLIENT_ORLANDOVIOLS
#define OV_CLIENT_ORLANDOVIOLS

#include "ov_types.h"
#include <thread>

class ov_client_orlandoviols_t : public ov_client_base_t {
public:
  ov_client_orlandoviols_t(ov_render_base_t& backend, const std::string& lobby);
  void start_service();
  void stop_service();

private:
  void service();
  std::string get_device_init(std::string url, const std::string& device,
                              std::string& hash);
  bool runservice;
  std::thread servicethread;
  std::string lobby;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
