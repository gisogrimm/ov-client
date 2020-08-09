#ifndef OV_TYPES
#define OV_TYPES

/// this file defines types for cartesian coordinates (pos_t) and for
/// Euler angles (zyx_euler_t):
#include <tascar/coordinates.h>

bool operator!=(const TASCAR::pos_t& a, const TASCAR::pos_t& b);

bool operator!=(const TASCAR::zyx_euler_t& a, const TASCAR::zyx_euler_t& b);

/// packet sequence type:
typedef int16_t sequence_t;
/// port number type:
typedef uint16_t port_t;
/// pin code type:
typedef uint32_t secret_t;
/// stage device id type:
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
  /// sender jitter:
  double senderjitter;
  /// receiver jitter:
  double receiverjitter;
};

bool operator!=(const std::vector<device_channel_t>& a,
                const std::vector<device_channel_t>& b);

bool operator!=(const stage_device_t& a, const stage_device_t& b);

struct render_settings_t {
  stage_device_id_t id;
  /// room dimensions:
  TASCAR::pos_t roomsize;
  /// average wall absorption coefficient:
  double absorption;
  /// damping coefficient, defines frequency tilt of T60:
  double damping;
  /// linear gain of reverberation:
  double reverbgain;
  /// flag wether rendering of reverb is required or not:
  bool renderreverb;
  /// flag wether rendering of virtual acoustics is required:
  bool rawmode;
  /// Receiver type, either ortf or hrtf:
  std::string rectype;
  /// self monitor gain:
  double egogain;
  /// peer-to-peer mode:
  bool peer2peer;
  /// output ports of device master:
  std::string outputport1;
  std::string outputport2;
  /// jitterbuffersize for second data receiver (e.g., for recording or
  /// broadcasting):
  double secrec;
};

bool operator!=(const render_settings_t& a, const render_settings_t& b);

struct stage_t {
  /// relay server host name:
  std::string host;
  /// relay server port:
  port_t port;
  /// relay server PIN:
  secret_t pin;
  /// rendering settings:
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
  ov_render_base_t(const std::string& deviceid);
  virtual ~ov_render_base_t(){};
  virtual void set_relay_server(const std::string& host, port_t port,
                                secret_t pin);
  virtual void start_session();
  virtual void end_session();
  virtual void configure_audio_backend(const audio_device_t&);
  virtual void add_stage_device(const stage_device_t& stagedevice);
  virtual void rm_stage_device(stage_device_id_t stagedeviceid);
  virtual void clear_stage();
  virtual void set_stage_device_gain(stage_device_id_t stagedeviceid,
                                     double gain);
  virtual void set_render_settings(const render_settings_t& rendersettings,
                                   stage_device_id_t thisstagedeviceid);
  virtual void start_audiobackend();
  virtual void stop_audiobackend();
  const bool is_session_active() const;
  const bool is_audio_active() const;
  const std::string& get_deviceid() const;

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
