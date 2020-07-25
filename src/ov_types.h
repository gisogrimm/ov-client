#ifndef OV_TYPES
#define OV_TYPES

#include <tascar/coordinates.h>

bool operator!=(const TASCAR::pos_t& a, const TASCAR::pos_t& b);

bool operator!=(const TASCAR::zyx_euler_t& a, const TASCAR::zyx_euler_t& b);

typedef uint8_t stage_device_id_t;

struct audio_device_t {
  /// driver name, e.g. jack, ALSA, ASIO
  std::string drivername;
  /// string represenation of device identifier, e.g., hw:1 for ALSA or jack
  std::string devicename;
  /// sampling rate in Hz
  double srate;
  /// number of samples in one period
  unsigned int periodsize;
  /// number of buffers
  unsigned int numperiods;
};

bool operator!=(const audio_device_t& a, const audio_device_t& b);

struct device_channel_t {
  /// Source of channel (used locally only):
  std::string sourceport;
  /// Linear playback gain:
  double gain;
  /// Position relative to stage device origin:
  TASCAR::pos_t position;
};

bool operator!=(const device_channel_t& a, const device_channel_t& b);

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

bool operator!=(const std::vector<device_channel_t>& a,
                const std::vector<device_channel_t>& b);

bool operator!=(const stage_device_t& a, const stage_device_t& b);

struct render_settings_t {
  stage_device_id_t id;
  /// room dimensions:
  TASCAR::pos_t roomsize;
  ///
  double absorption;
  double damping;
  bool renderreverb;
  bool rawmode;
  /// Receiver type, either ortf or hrtf:
  std::string rectype;
  /// self monitor gain:
  double egogain;
};

bool operator!=(const render_settings_t& a, const render_settings_t& b);

struct stage_t {
  render_settings_t rendersettings;
  /// Device identifier of this stage device (typically its mac address):
  std::string thisdeviceid;
  /// Numeric identifier of this device within the stage:
  stage_device_id_t thisstagedeviceid;
  /// Devices on the stage:
  std::map<stage_device_id_t, stage_device_t> stage;
};

class ov_render_base_t {
public:
  ov_render_base_t(const std::string& deviceid)
      : audiodevice({"", "", 48000, 96, 2}), stage({{}, deviceid, 0}),
        session_active(false), audio_active(false){};
  virtual ~ov_render_base_t(){};
  virtual void start_session() { session_active = true; };
  virtual void end_session() { session_active = false; };
  virtual void configure_audio_backend(const audio_device_t&);
  virtual void add_stage_device(const stage_device_t& stagedevice);
  virtual void rm_stage_device(stage_device_id_t stagedeviceid);
  virtual void clear_stage();
  virtual void set_stage_device_gain(stage_device_id_t stagedeviceid,
                                     double gain);
  virtual void set_render_settings(const render_settings_t& rendersettings);
  virtual void start_audiobackend() { audio_active = true; };
  virtual void stop_audiobackend() { audio_active = false; };
  const bool is_session_active() const { return session_active; };
  const bool is_audio_active() const { return audio_active; };
  const std::string& get_deviceid() const { return stage.thisdeviceid; };

protected:
  audio_device_t audiodevice;
  stage_t stage;

private:
  bool session_active;
  bool audio_active;
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

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
