#ifndef OV_DS_SERVICE
#define OV_DS_SERVICE

#include "ov_types.h"
#include <atomic>
#include <thread>

class ov_ds_service_t {
public:
  ov_ds_service_t(const std::string& api_url);
  void start(const std::string& token);
  void stop();

private:
  void service();

  bool running_;
  std::thread servicethread_;
  std::string api_url_;
  std::string token_;
  std::atomic<bool> quitrequest_;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
