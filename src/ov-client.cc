#include <stdint.h>
#include <string>

#include "ov_types.h"

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

int main(int argc, char** argv)
{
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
