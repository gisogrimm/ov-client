#include <stdint.h>
#include <string>
#include <tascar/coordinates.h>

typedef uint8_t stage_device_id_t;

struct device_channel_t {
  /// Source of channel (used locally only):
  std::string sourceport;
  /// Linear playback gain:
  double gain;
  /// Position relative to stage device origin:
  TASCAR::pos_t position;
};

struct stage_device_t {
  /// ID within the stage, typically a number from 0 to number of stage devices:
  stage_device_id_t id;
  /// Label of the stage device:
  std::string label;
  /// List of channels the device is providing:
  std::vector<device_channel_t> channels;
  /// Position of the stage device in the virtual space:
  TASCAR::pos_t position;
  /// Orientation of the stage device in the virtual space, ZYX Euler angles:
  TASCAR::zyx_euler_t orientation;
  /// Linear gain of the stage device:
  double gain;
  /// Mute flag:
  bool mute;
};

class ov_render_base_t {
public:
  ov_render_base_t(){};
  virtual ~ov_render_base_t(){};
  void start_session(){};
  void end_session(){};
  void configure_audio_backend(const std::string& device, double srate,
                               unsigned int periodsize, unsigned int buffers){};
  void add_stage_device(const stage_device_t& stagedevice){};
  void rm_stage_device(stage_device_id_t stagedeviceid){};
  void set_stage_device_gain(stage_device_id_t stagedeviceid, double gain){};
};

class ov_client_base_t {
public:
  ov_client_base_t(ov_render_base_t& backend) : backend(backend){};
  virtual ~ov_client_base_t(){};
  virtual void start_service(){};
  virtual void stop_service(){};

protected:
  ov_render_base_t& backend;
};

class ov_render_tascar_t : public ov_render_base_t {
public:
  ov_render_tascar_t();
  ~ov_render_tascar_t();
  void start_session();
  void end_session();
  void configure_audio_backend(const std::string& device, double srate,
                               unsigned int periodsize, unsigned int buffers);
  void add_stage_device(stage_device_id_t stagedeviceid,
                        const std::string& name, unsigned int numchannels);
  void rm_stage_device(stage_device_id_t stagedeviceid);
  void set_stage_device_gain(stage_device_id_t stagedeviceid, double gain);
};

int main(int argc, char** argv)
{
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
