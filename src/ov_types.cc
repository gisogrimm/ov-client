#include "ov_types.h"

bool operator!=(const TASCAR::pos_t& a, const TASCAR::pos_t& b)
{
  return (a.x != b.x) || (a.y != b.y) || (a.z != b.z);
}

bool operator!=(const TASCAR::zyx_euler_t& a, const TASCAR::zyx_euler_t& b)
{
  return (a.x != b.x) || (a.y != b.y) || (a.z != b.z);
}

bool operator!=(const audio_device_t& a, const audio_device_t& b)
{
  return (a.drivername != b.drivername) || (a.devicename != b.devicename) ||
         (a.srate != b.srate) || (a.periodsize != b.periodsize) ||
         (a.numperiods != b.numperiods);
}

bool operator!=(const device_channel_t& a, const device_channel_t& b)
{
  return (a.sourceport != b.sourceport) || (a.gain != b.gain) ||
         (a.position != b.position);
}

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

bool operator!=(const render_settings_t& a, const render_settings_t& b)
{
  return (a.roomsize != b.roomsize) || (a.absorption != b.absorption) ||
         (a.damping != b.damping) || (a.renderreverb != b.renderreverb) ||
         (a.rawmode != b.rawmode) || (a.rectype != b.rectype) ||
         (a.egogain != b.egogain);
}

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

void ov_render_base_t::clear_stage()
{
  stage.stage.clear();
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

void ov_render_base_t::set_render_settings(
    const render_settings_t& rendersettings)
{
  stage.rendersettings = rendersettings;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
