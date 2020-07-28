#include "ov_render_tascar.h"
#include <unistd.h>

// std::string to_string(const TASCAR::pos_t& x)
//{
//  return TASCAR::to_string(x.x) + " " + TASCAR::to_string(x.y) + " " +
//         TASCAR::to_string(x.z);
//}
//
// std::string to_string(const TASCAR::zyx_euler_t& x)
//{
//  return TASCAR::to_string(x.z) + " " + TASCAR::to_string(x.y) + " " +
//         TASCAR::to_string(x.x);
//}

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

void ov_render_tascar_t::start_session(const std::string& host, port_t port)
{
  ov_render_base_t::start_session(host, port);
  tascar = new TASCAR::session_t(
      "<?xml version=\"1.0\"?><session srv_port=\"none\"/>",
      TASCAR::session_t::LOAD_STRING, "");
  try {
    xmlpp::Element* e_session(tascar->doc->get_root_node());
    e_session->set_attribute("duration", "36000");
    e_session->set_attribute("name", stage.thisdeviceid);
    e_session->set_attribute("license", "CC0");
    e_session->set_attribute("levelmeter_tc", "0.5");
    if(!stage.rendersettings.rawmode) {
      xmlpp::Element* e_scene(e_session->add_child("scene"));
      e_scene->set_attribute("name", stage.thisdeviceid);
      xmlpp::Element* e_rec = e_scene->add_child("receiver");
      e_rec->set_attribute("type", stage.rendersettings.rectype);
      if(stage.rendersettings.rectype == "ortf") {
        e_rec->set_attribute("angle", "140");
        e_rec->set_attribute("f6db", "12000");
        e_rec->set_attribute("fmin", "3000");
      }
      e_rec->set_attribute("name", "master");
      e_rec->set_attribute("delaycomp", "0.05");
      e_rec->set_attribute(
          "dlocation",
          TASCAR::to_string(stage.stage[stage.rendersettings.id].position));
      e_rec->set_attribute(
          "dorientation",
          TASCAR::to_string(stage.stage[stage.rendersettings.id].orientation));
      DEBUG(stage.host);
      DEBUG(stage.port);
      if(!stage.stage.empty()) {
        DEBUG(stage.stage.size());
        // the stage is not empty, which means we are on a stage.
        // b_sender is true if this device is sending audio. If this
        // device is not sending audio, then the stage layout will
        // differ.
        bool b_sender(!stage.stage[stage.thisstagedeviceid].channels.empty());
        DEBUG(b_sender);
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
          DEBUG((int)(stagemember.first));
          DEBUG(stagemember.second.label);
          az += daz;
          TASCAR::pos_t pos(stagemember.second.position);
          TASCAR::zyx_euler_t rot(stagemember.second.orientation);
          if(!b_sender) {
            pos.x = radius * cos(az);
            pos.y = -radius * sin(az);
            pos.z = 0;
            rot.z = (180 / M_PI * (-az + M_PI));
            rot.y = 0;
            rot.x = 0;
          }
          xmlpp::Element* e_src(e_scene->add_child("source"));
          TASCAR::pos_t ego_delta;
          if(stagemember.second.id == stage.thisstagedeviceid) {
            e_src->set_attribute("name", "ego");
            ego_delta.x = 0.2;
            ego_delta.z = -0.3;
          } else {
            e_src->set_attribute("name", stagemember.second.label);
          }
          if(b_sender) {
            e_src->set_attribute("dlocation", to_string(pos));
            e_src->set_attribute("dorientation", to_string(rot));
          }
          for(auto ch : stagemember.second.channels) {
            xmlpp::Element* e_snd(e_src->add_child("sound"));
            if(stagemember.second.id == stage.thisstagedeviceid)
              e_snd->set_attribute("connect", ch.sourceport);
            e_snd->set_attribute("gain",
                                 TASCAR::to_string(20.0 * log10(ch.gain)));
            e_snd->set_attribute("x", TASCAR::to_string(ch.position.x));
            e_snd->set_attribute("y", TASCAR::to_string(ch.position.y));
            e_snd->set_attribute("z", TASCAR::to_string(ch.position.z));
          }
        }
      } else {
        // the stage is empty, which means we play an announcement only.
      }
    }
    tascar->doc->write_to_file_formatted("debugsession.tsc");
    tascar->start();
  }
  catch(...) {
    delete tascar;
    tascar = NULL;
    end_session();
    throw;
  }
}

void ov_render_tascar_t::end_session()
{
  ov_render_base_t::end_session();
  if(tascar) {
    tascar->stop();
    delete tascar;
  }
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
    start_session(stage.host, stage.port);
  }
}

void ov_render_tascar_t::rm_stage_device(stage_device_id_t stagedeviceid)
{
  ov_render_base_t::rm_stage_device(stagedeviceid);
  if(is_session_active()) {
    end_session();
    start_session(stage.host, stage.port);
  }
}

void ov_render_tascar_t::set_stage_device_gain(stage_device_id_t stagedeviceid,
                                               double gain)
{
  ov_render_base_t::set_stage_device_gain(stagedeviceid, gain);
  // this is really brute force, will be replaced soon:
  if(is_session_active()) {
    end_session();
    start_session(stage.host, stage.port);
  }
}

void ov_render_tascar_t::set_render_settings(
    const render_settings_t& rendersettings)
{
  if(rendersettings != stage.rendersettings) {
    ov_render_base_t::set_render_settings(rendersettings);
    if(is_session_active()) {
      end_session();
      start_session(stage.host, stage.port);
    }
  }
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
