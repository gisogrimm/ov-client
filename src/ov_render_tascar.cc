#include "ov_render_tascar.h"
#include <unistd.h>

ov_render_tascar_t::ov_render_tascar_t(const std::string& deviceid)
    : ov_render_base_t(deviceid), h_pipe_jack(NULL), tascar(NULL)
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
    if(!stage.stage.empty()) {
      // the stage is not empty, which means we are on a stage.

      // b_sender is true if this device is sending audio. If this
      // device is not sending audio, then the stage layout will
      // differ.
      bool b_sender(!stage.stage[stage.thisstagedeviceid].channels.empty());
      // width of stage in degree:
      double stagewidth(160);
      double az(-0.5 * stagewidth);
      if(b_sender) {
        stagewidth = 360;
        az = 0;
      }
      double daz(stagewidth / stage.stage.size() * (M_PI / 180.0));
      az = az * (M_PI / 180.0) - 0.5 * daz;
      double radius(1.2);
      for(auto stagemember : stage.stage) {
        az += daz;

        xmlpp::Element* e_src(e_scene->add_child("source"));
        e_src->set_attribute("name", "true");
        xmlpp::Element* e_snd(e_src->add_child("sound"));
      }
    } else {
      // the stage is empty, which means we play an announcement only.
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

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
