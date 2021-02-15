#ifndef OV_CLIENT_DIGITALSTAGE
#define OV_CLIENT_DIGITALSTAGE

#include "ov_types.h"
#include <atomic>
#include <thread>

class ov_client_digitalstage_t : public ov_client_base_t {
public:
  ov_client_digitalstage_t(ov_render_base_t& backend,
                           const std::string& api_url,
                           const std::string& token);
  void start_service();
  void stop_service();
  bool download_file(const std::string& url, const std::string& dest);
  bool is_going_to_stop() const { return quitrequest_; };

private:
  void service();

  bool runservice_;
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
