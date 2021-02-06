#ifndef OV_CLIENT_ORLANDOVIOLS
#define OV_CLIENT_ORLANDOVIOLS

#include "ov_types.h"
#include <atomic>
#include <thread>

class ov_client_orlandoviols_t : public ov_client_base_t {
public:
  ov_client_orlandoviols_t(ov_render_base_t& backend, const std::string& lobby);
  void start_service();
  void stop_service();
  bool download_file(const std::string& url, const std::string& dest);
  bool is_going_to_stop() const { return quitrequest_; };

private:
  void service();
  void register_device(std::string url, const std::string& device);
  std::string device_update(std::string url, const std::string& device,
                            std::string& hash);
  bool report_error(std::string url, const std::string& device,
                    const std::string& msg);

  bool runservice;
  std::thread servicethread;
  std::string lobby;
  std::atomic<bool> quitrequest_;
  bool isovbox;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
