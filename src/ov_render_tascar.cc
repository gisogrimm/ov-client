#include "ov_render_tascar.h"
#include <unistd.h>

ov_render_tascar_t::ov_render_tascar_t(const std::string& deviceid)
    : ov_render_base_t(deviceid), h_pipe_jack(NULL), tascar(NULL),
      ovboxclient(NULL)
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

void ov_render_tascar_t::create_virtual_acoustics(xmlpp::Element* e_session,
                                                  xmlpp::Element* e_rec,
                                                  xmlpp::Element* e_scene)
{
  stage_device_t& thisdev(stage.stage[stage.thisstagedeviceid]);
  // list of ports on which TASCAR will wait before attempting to connect:
  std::vector<std::string> waitports;
  // set position and orientation of receiver:
  e_rec->set_attribute(
      "dlocation",
      TASCAR::to_string(stage.stage[stage.rendersettings.id].position));
  e_rec->set_attribute(
      "dorientation",
      TASCAR::to_string(stage.stage[stage.rendersettings.id].orientation));
  // the stage is not empty, which means we are on a stage.
  // b_sender is true if this device is sending audio. If this
  // device is not sending audio, then the stage layout will
  // differ.
  bool b_sender(!thisdev.channels.empty());
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
  //
  for(auto stagemember : stage.stage) {
    az += daz;
    TASCAR::pos_t pos(stagemember.second.position);
    TASCAR::zyx_euler_t rot(stagemember.second.orientation);
    if(!b_sender) {
      // overwrite stage layout:
      pos.x = radius * cos(az);
      pos.y = -radius * sin(az);
      pos.z = 0;
      rot.z = (180 / M_PI * (-az + M_PI));
      rot.y = 0;
      rot.x = 0;
    }
    // create a sound source for each device on stage:
    xmlpp::Element* e_src(e_scene->add_child("source"));
    TASCAR::pos_t ego_delta;
    if(stagemember.second.id == thisdev.id) {
      // in case of self-monitoring, this source is called "ego", and the
      // position is slightly shifted:
      e_src->set_attribute("name", "ego");
      ego_delta.x = 0.2;
      ego_delta.z = -0.3;
    } else {
      e_src->set_attribute("name", get_stagedev_name(stagemember.second.id));
    }
    e_src->set_attribute("dlocation", to_string(pos));
    e_src->set_attribute("dorientation", to_string(rot));
    uint32_t kch(0);
    for(auto ch : stagemember.second.channels) {
      // create a sound for each channel:
      ++kch;
      xmlpp::Element* e_snd(e_src->add_child("sound"));
      e_snd->set_attribute("maxdist", "50");
      e_snd->set_attribute("gainmodel", "1");
      double gain(ch.gain * stagemember.second.gain);
      if(stagemember.second.id == thisdev.id) {
        // connect self-monitoring source ports:
        e_snd->set_attribute("connect", ch.sourceport);
        gain *= stage.rendersettings.egogain;
      } else {
        // if not self-monitor then decrease gain:
        gain *= 0.6;
      }
      e_snd->set_attribute("gain", TASCAR::to_string(20.0 * log10(gain)));
      // set relative channel positions:
      TASCAR::pos_t chpos(ch.position);
      chpos += ego_delta;
      e_snd->set_attribute("x", TASCAR::to_string(chpos.x));
      e_snd->set_attribute("y", TASCAR::to_string(chpos.y));
      e_snd->set_attribute("z", TASCAR::to_string(chpos.z));
    }
  }
  if(stage.rendersettings.renderreverb) {
    // create reverb engine:
    xmlpp::Element* e_rvb(e_scene->add_child("reverb"));
    e_rvb->set_attribute("type", "simplefdn");
    e_rvb->set_attribute("volumetric",
                         TASCAR::to_string(stage.rendersettings.roomsize));
    e_rvb->set_attribute("image", "false");
    e_rvb->set_attribute("fdnorder", "5");
    e_rvb->set_attribute("dw", "60");
    e_rvb->set_attribute("absorption",
                         TASCAR::to_string(stage.rendersettings.absorption));
    e_rvb->set_attribute("damping",
                         TASCAR::to_string(stage.rendersettings.damping));
    e_rvb->set_attribute(
        "gain", TASCAR::to_string(20 * log10(stage.rendersettings.reverbgain)));
  }
  // configure extra modules:
  xmlpp::Element* e_mods(e_session->add_child("modules"));
  if(!stage.rendersettings.rawmode) {
    // add web mixer tools (node-js server and touchosc interface):
    xmlpp::Element* e_node(e_mods->add_child("system"));
    e_node->set_attribute("command", "node webmixer.js");
    e_mods->add_child("touchosc");
  }
  //
  for(auto stagemember : stage.stage) {
    std::string chanlist;
    for(uint32_t k = 0; k < stagemember.second.channels.size(); ++k) {
      if(k)
        chanlist += ",";
      chanlist += std::to_string(k + 1);
    }
    if(stage.thisstagedeviceid != stagemember.second.id) {
      std::string clientname(get_stagedev_name(stagemember.second.id));
      xmlpp::Element* e_sys = e_mods->add_child("system");
      double buff(thisdev.receiverjitter + stagemember.second.senderjitter);
      e_sys->set_attribute(
          "command", "zita-n2j --chan " + chanlist + " --jname " + clientname +
                         " --buf " + TASCAR::to_string(buff) + " 0.0.0.0 " +
                         TASCAR::to_string(4464 + 2 * stagemember.second.id));
      e_sys->set_attribute("onunload", "killall zita-n2j");
      for(size_t c = 0; c < stagemember.second.channels.size(); ++c) {
        if(stage.thisstagedeviceid != stagemember.second.id) {
          std::string srcport(clientname + ":out_" + std::to_string(c + 1));
          std::string destport("render." + stage.thisdeviceid + ":" +
                               clientname + "." + std::to_string(c) + ".0");
          waitports.push_back(srcport);
          xmlpp::Element* e_port = e_session->add_child("connect");
          e_port->set_attribute("src", srcport);
          e_port->set_attribute("dest", destport);
        }
      }
      if(stage.rendersettings.secrec > 0) {
        if(stage.thisstagedeviceid != stagemember.second.id) {
          std::string clientname(get_stagedev_name(stagemember.second.id) +
                                 "_sec");
          std::string netclientname(
              "n2j_" + std::to_string(stagemember.second.id) + "_sec");
          xmlpp::Element* e_sys = e_mods->add_child("system");
          double buff(thisdev.receiverjitter + stagemember.second.senderjitter);
          e_sys->set_attribute(
              "command",
              "zita-n2j --chan " + chanlist + " --jname " + netclientname +
                  " --buf " +
                  TASCAR::to_string(stage.rendersettings.secrec + buff) +
                  " 0.0.0.0 " +
                  TASCAR::to_string(4464 + 2 * stagemember.second.id + 100));
          e_sys->set_attribute("onunload", "killall zita-n2j");
          xmlpp::Element* e_route = e_mods->add_child("route");
          e_route->set_attribute("name", clientname);
          e_route->set_attribute(
              "channels", std::to_string(stagemember.second.channels.size()));
          e_route->set_attribute(
              "gain", TASCAR::to_string(20 * log10(stagemember.second.gain)));
          e_route->set_attribute("connect", netclientname + ":out_[0-9]*");
        }
      }
    }
  }
  // when a second network receiver is used then also create a bus
  // with a delayed version of the self monitor:
  if(stage.rendersettings.secrec > 0) {
    std::string clientname(get_stagedev_name(thisdev.id) + "_sec");
    xmlpp::Element* mod = e_mods->add_child("route");
    mod->set_attribute("name", clientname);
    mod->set_attribute("channels", std::to_string(thisdev.channels.size()));
    xmlpp::Element* plgs = mod->add_child("plugins");
    xmlpp::Element* del = plgs->add_child("delay");
    del->set_attribute("delay",
                       TASCAR::to_string(0.001 * (stage.rendersettings.secrec +
                                                  thisdev.receiverjitter +
                                                  thisdev.senderjitter)));
    mod->set_attribute("gain", TASCAR::to_string(20 * log10(thisdev.gain)));
    int chn(0);
    for(auto ch : thisdev.channels) {
      xmlpp::Element* e_port = e_session->add_child("connect");
      e_port->set_attribute("src", ch.sourceport);
      e_port->set_attribute("dest", clientname + ":in." + std::to_string(chn));
      ++chn;
    }
  }
  if(thisdev.channels.size() > 0) {
    xmlpp::Element* e_sys = e_mods->add_child("system");
    e_sys->set_attribute(
        "command", "zita-j2n --chan " +
                       std::to_string(thisdev.channels.size()) +
                       " --jname sender --16bit 127.0.0.1 " +
                       std::to_string(4464 + 2 * stage.thisstagedeviceid));
    e_sys->set_attribute("onunload", "killall zita-j2n");
    int chn(0);
    for(auto ch : thisdev.channels) {
      ++chn;
      xmlpp::Element* e_port = e_session->add_child("connect");
      e_port->set_attribute("src", ch.sourceport);
      e_port->set_attribute("dest", "sender:in_" + std::to_string(chn));
    }
  }
  xmlpp::Element* e_wait = e_mods->add_child("waitforjackport");
  for(auto port : waitports) {
    xmlpp::Element* e_p = e_wait->add_child("port");
    e_p->add_child_text(port);
  }
}

void ov_render_tascar_t::clear_stage()
{
  ov_render_base_t::clear_stage();
  if(is_session_active()) {
    end_session();
  }
}

void ov_render_tascar_t::start_session()
{
  // do whatever needs to be done in base class:
  ov_render_base_t::start_session();
  // create a short link to this device:
  try {
    // xml code for TASCAR configuration:
    TASCAR::xml_doc_t tsc;
    // default TASCAR session settings:
    tsc.doc->create_root_node("session");
    xmlpp::Element* e_session(tsc.doc->get_root_node());
    e_session->set_attribute("srv_port", "9871");
    e_session->set_attribute("duration", "36000");
    e_session->set_attribute("name", stage.thisdeviceid);
    e_session->set_attribute("license", "CC0");
    e_session->set_attribute("levelmeter_tc", "0.5");
    // create virtual acoustics only when not in raw mode:
    if(!stage.rendersettings.rawmode) {
      // create a virtual acoustics "scene":
      xmlpp::Element* e_scene(e_session->add_child("scene"));
      e_scene->set_attribute("name", stage.thisdeviceid);
      // add a main receiver for which the scene is rendered:
      xmlpp::Element* e_rec = e_scene->add_child("receiver");
      // receiver can be "hrtf" or "ortf" (more receivers are possible in
      // TASCAR, but only these two can produce audio which is
      // headphone-compatible):
      e_rec->set_attribute("type", stage.rendersettings.rectype);
      if(stage.rendersettings.rectype == "ortf") {
        e_rec->set_attribute("angle", "140");
        e_rec->set_attribute("f6db", "12000");
        e_rec->set_attribute("fmin", "3000");
      }
      e_rec->set_attribute("name", "master");
      // do not add delay (usually used for dynamic scene rendering):
      e_rec->set_attribute("delaycomp", "0.05");
      // connect output ports:
      if(!stage.rendersettings.outputport1.empty()) {
        xmlpp::Element* e_con(e_session->add_child("connect"));
        e_con->set_attribute("src",
                             "render." + stage.thisdeviceid + ":master_l");
        e_con->set_attribute("dest", stage.rendersettings.outputport1);
      }
      if(!stage.rendersettings.outputport2.empty()) {
        xmlpp::Element* e_con(e_session->add_child("connect"));
        e_con->set_attribute("src",
                             "render." + stage.thisdeviceid + ":master_r");
        e_con->set_attribute("dest", stage.rendersettings.outputport2);
      }
      // the host is not empty when this device is on a stage:
      if(!stage.host.empty()) {
        create_virtual_acoustics(e_session, e_rec, e_scene);
      } else {
        // the stage is empty, which means we play an announcement only.
        xmlpp::Element* e_src(e_scene->add_child("source"));
        e_src->set_attribute("name", "announce");
        xmlpp::Element* e_snd(e_src->add_child("sound"));
        e_snd->set_attribute("maxdist", "50");
        e_snd->set_attribute("gainmodel", "1");
        e_snd->set_attribute("x", "4");
        xmlpp::Element* e_plugs(e_snd->add_child("plugins"));
        xmlpp::Element* e_sndfile(e_plugs->add_child("sndfile"));
        e_sndfile->set_attribute("name", "announce.flac");
        e_sndfile->set_attribute("level", "57");
        e_sndfile->set_attribute("transport", "false");
        e_sndfile->set_attribute("loop", "0");
      }
    }
    if(!stage.host.empty()) {
      ovboxclient = new ovboxclient_t(
          stage.host, stage.port, 4464 + 2 * stage.thisstagedeviceid, 0, 30,
          stage.pin, stage.thisstagedeviceid, stage.rendersettings.peer2peer,
          false, false);
      if(stage.rendersettings.secrec > 0)
        ovboxclient->add_extraport(100);
    }
    tsc.doc->write_to_file_formatted("debugsession.tsc");
    tascar = new TASCAR::session_t(tsc.doc->write_to_string(),
                                   TASCAR::session_t::LOAD_STRING, "");
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
    tascar = NULL;
  }
  if(ovboxclient) {
    delete ovboxclient;
    ovboxclient = NULL;
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
  if(is_session_active() && tascar) {
    uint32_t k = 0;
    for(auto ch : stage.stage[stagedeviceid].channels) {
      gain = ch.gain * stage.stage[stagedeviceid].gain;
      if(stagedeviceid == stage.thisstagedeviceid) {
        gain *= stage.rendersettings.egogain;
      } else {
        // if not self-monitor then decrease gain:
        gain *= 0.6;
      }
      std::string pattern("/" + stage.thisdeviceid + "/" +
                          get_stagedev_name(stagedeviceid) + "/" +
                          TASCAR::to_string(k));
      std::vector<TASCAR::Scene::audio_port_t*> port(
          tascar->find_audio_ports(std::vector<std::string>(1, pattern)));
      if(port.size())
        port[0]->set_gain_lin(gain);
      ++k;
    }
  }
}

void ov_render_tascar_t::set_render_settings(
    const render_settings_t& rendersettings)
{
  if(rendersettings != stage.rendersettings) {
    ov_render_base_t::set_render_settings(rendersettings);
    if(is_session_active()) {
      end_session();
      start_session();
    }
  }
}

std::string ov_render_tascar_t::get_stagedev_name(stage_device_id_t id) const
{
  auto stagedev(stage.stage.find(id));
  if(stagedev == stage.stage.end())
    return "";
  return stagedev->second.label + "_" + TASCAR::to_string(id);
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
