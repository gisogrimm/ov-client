#include <stdint.h>
#include <string>
#include <tascar/coordinates.h>
#include <tascar/session.h>
#include <unistd.h>

bool operator!=(const TASCAR::pos_t& a, const TASCAR::pos_t& b)
{
  return (a.x != b.x) || (a.y != b.y) || (a.z != b.z);
}

bool operator!=(const TASCAR::zyx_euler_t& a, const TASCAR::zyx_euler_t& b)
{
  return (a.x != b.x) || (a.y != b.y) || (a.z != b.z);
}

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

bool operator!=(const audio_device_t& a, const audio_device_t& b)
{
  return (a.drivername != b.drivername) || (a.devicename != b.devicename) ||
         (a.srate != b.srate) || (a.periodsize != b.periodsize) ||
         (a.numperiods != b.numperiods);
}

struct device_channel_t {
  /// Source of channel (used locally only):
  std::string sourceport;
  /// Linear playback gain:
  double gain;
  /// Position relative to stage device origin:
  TASCAR::pos_t position;
};

bool operator!=(const device_channel_t& a, const device_channel_t& b)
{
  return (a.sourceport != b.sourceport) || (a.gain != b.gain) ||
         (a.position != b.position);
}

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
                const std::vector<device_channel_t>& b)
{
  if(a.size() != b.size())
    return true;
  for(size_t k = 0; k < a.size(); ++k)
    if(a[k] != b[k])
      return true;
  return false;
}

bool operator!=(const stage_device_t& a, const stage_device_t& b)
{
  return (a.id != b.id) || (a.label != b.label) || (a.channels != b.channels) ||
         (a.position != b.position) || (a.orientation != b.orientation) ||
         (a.gain != b.gain) || (a.mute != b.mute);
}

struct stage_t {
  /// Device identifier of this stage device (typically its mac address):
  std::string thisdeviceid;
  /// Numeric identifier of this device within the stage:
  stage_device_id_t thisstagedeviceid;
  /// Devices on the stage:
  std::map<stage_device_id_t, stage_device_t> stage;
  /// room dimensions:
  TASCAR::pos_t roomsize;
  ///
  double absorption;
  double damping;
  bool renderreverb;
  bool rawmode;
  /// Receiver type, either ortf or hrtf:
  std::string rectype;
};

class ov_render_base_t {
public:
  ov_render_base_t()
      : audiodevice({"", "", 48000, 96, 2}), session_active(false),
        audio_active(false){};
  virtual ~ov_render_base_t(){};
  virtual void start_session() { session_active = true; };
  virtual void end_session() { session_active = false; };
  virtual void configure_audio_backend(const audio_device_t&);
  virtual void add_stage_device(const stage_device_t& stagedevice);
  virtual void rm_stage_device(stage_device_id_t stagedeviceid);
  virtual void set_stage_device_gain(stage_device_id_t stagedeviceid,
                                     double gain);
  virtual void start_audiobackend() { audio_active = true; };
  virtual void stop_audiobackend() { audio_active = false; };
  const bool is_session_active() const { return session_active; };
  const bool is_audio_active() const { return audio_active; };

protected:
  audio_device_t audiodevice;
  stage_t stage;

private:
  bool session_active;
  bool audio_active;
};

void ov_render_base_t::configure_audio_backend(
    const audio_device_t& audiodevice_)
{
  if(audiodevice != audiodevice_) {
    audiodevice = audiodevice_;
    if(audio_active) {
      bool session_was_active(session_active);
      if(session_active)
        end_session();
      stop_audiobackend();
      start_audiobackend();
      if(session_was_active)
        start_session();
    }
  }
}

void ov_render_base_t::add_stage_device(const stage_device_t& stagedevice)
{
  stage.stage[stagedevice.id] = stagedevice;
}

void ov_render_base_t::rm_stage_device(stage_device_id_t stagedeviceid)
{
  stage.stage.erase(stagedeviceid);
}

void ov_render_base_t::set_stage_device_gain(stage_device_id_t stagedeviceid,
                                             double gain)
{
  if(stage.stage.find(stagedeviceid) != stage.stage.end())
    stage.stage[stagedeviceid].gain = gain;
}

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
  void start_audiobackend();
  void stop_audiobackend();
  void add_stage_device(const stage_device_t& stagedevice);
  void rm_stage_device(stage_device_id_t stagedeviceid);
  void set_stage_device_gain(stage_device_id_t stagedeviceid, double gain);

private:
  FILE* h_pipe_jack;
  TASCAR::session_t* tascar;
};

ov_render_tascar_t::ov_render_tascar_t() : h_pipe_jack(NULL), tascar(NULL)
{
  audiodevice = {"jack", "hw:1", 48000, 96, 2};
}

ov_render_tascar_t::~ov_render_tascar_t()
{
  if(is_session_active())
    end_session();
  if(is_audio_active())
    stop_audiobackend();
}

void ov_render_tascar_t::start_session()
{
  ov_render_base_t::start_session();
  tascar = new TASCAR::session_t(
      "<?xml version=\"1.0\"?><session srv_port=\"none\"/>",
      TASCAR::session_t::LOAD_STRING, "");
  xmlpp::Element* e_session(tascar->doc->get_root_node());
  e_session->set_attribute("duration", "36000");
  e_session->set_attribute("name", stage.thisdeviceid);
  e_session->set_attribute("license", "CC0");
  e_session->set_attribute("levelmeter_tc", "0.5");
  if(!stage.rawmode) {
    xmlpp::Element* e_scene(e_session->add_child("scene"));
    e_scene->set_attribute("name", stage.thisdeviceid);
    xmlpp::Element* e_rec = e_scene->add_child("receiver");
    e_rec->set_attribute("type", stage.rectype);
    if(stage.rectype == "ortf") {
      e_rec->set_attribute("angle", "140");
      e_rec->set_attribute("f6db", "12000");
      e_rec->set_attribute("fmin", "3000");
    }
    e_rec->set_attribute("name", "master");
    e_rec->set_attribute("delaycomp", "0.05");
    for(auto stagemember : stage.stage) {
      xmlpp::Element* e_src(e_scene->add_child("source"));
      e_src->set_attribute("name", "true");
      xmlpp::Element* e_snd(e_src->add_child("sound"));
    }
  }
  tascar->start();
}

void ov_render_tascar_t::end_session()
{
  ov_render_base_t::end_session();
  tascar->stop();
  delete tascar;
}

void ov_render_tascar_t::start_audiobackend()
{
  ov_render_base_t::start_audiobackend();
  if((audiodevice.drivername == "jack") &&
     (audiodevice.devicename != "manual")) {
    char cmd[1024];
    sprintf(cmd,
            "JACK_NO_AUDIO_RESERVATION=1 jackd --sync -P 40 -d alsa -d %s "
            "-r %g -p %d -n %d",
            audiodevice.devicename.c_str(), audiodevice.srate,
            audiodevice.periodsize, audiodevice.numperiods);
    h_pipe_jack = popen(cmd, "w");
    // replace sleep by testing for jack presence with timeout:
    sleep(4);
  }
}

void ov_render_tascar_t::stop_audiobackend()
{
  ov_render_base_t::stop_audiobackend();
  if(h_pipe_jack) {
    FILE* h_pipe(popen("killall jackd", "w"));
    fclose(h_pipe_jack);
    fclose(h_pipe);
  }
  h_pipe_jack = NULL;
}

void ov_render_tascar_t::add_stage_device(const stage_device_t& stagedevice)
{
  ov_render_base_t::add_stage_device(stagedevice);
  if(is_session_active()) {
    end_session();
    start_session();
  }
}

void ov_render_tascar_t::rm_stage_device(stage_device_id_t stagedeviceid)
{
  ov_render_base_t::rm_stage_device(stagedeviceid);
  if(is_session_active()) {
    end_session();
    start_session();
  }
}

void ov_render_tascar_t::set_stage_device_gain(stage_device_id_t stagedeviceid,
                                               double gain)
{
  ov_render_base_t::set_stage_device_gain(stagedeviceid, gain);
  // this is really brute force, will be replaced soon:
  if(is_session_active()) {
    end_session();
    start_session();
  }
}

int main(int argc, char** argv)
{
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
