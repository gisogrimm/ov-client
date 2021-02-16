#include "ov_render_tascar.h"
#include "soundcardtools.h"
#include <fstream>
#include <nlohmann/json.hpp>

bool ov_render_tascar_t::metronome_t::operator!=(const metronome_t& a)
{
  return (a.bpb != bpb) || (a.bpm != bpm) || (a.bypass != bypass) ||
         (a.delay != delay) || (a.level != level);
}

ov_render_tascar_t::metronome_t::metronome_t()
    : bpb(4), bpm(120.0), bypass(true), delay(40.0), level(65)
{
}

ov_render_tascar_t::metronome_t::metronome_t(nlohmann::json js)
    : bpb(my_js_value(js, "bpb", 4)), bpm(my_js_value(js, "bpm", 120.0)),
      bypass(!my_js_value(js, "active", false)),
      delay(my_js_value(js, "delay", 40.0)),
      level(my_js_value(js, "level", 65.0))
{
}

void ov_render_tascar_t::metronome_t::set_xmlattr(xmlpp::Element* em,
                                                  xmlpp::Element* ed) const
{
  em->set_attribute("bpm", std::to_string(bpm));
  em->set_attribute("bpb", std::to_string(bpb));
  em->set_attribute("bypass", (bypass ? "true" : "false"));
  ed->set_attribute("delay", "0 " + std::to_string(0.001 * delay));
  em->set_attribute("a1", std::to_string(level));
  em->set_attribute("ao", std::to_string(level));
}

void ov_render_tascar_t::metronome_t::update_osc(TASCAR::osc_server_t* srv,
                                                 const std::string& dev) const
{
  if(srv) {
    lo_message msg(lo_message_new());
    lo_message_add_float(msg, 1);
    lo_arg** loarg(lo_message_get_argv(msg));
    loarg[0]->f = bpm;
    srv->dispatch_data_message(
        (std::string("/") + dev + ".metronome/ap*/metronome/bpm").c_str(), msg);
    loarg[0]->f = bypass;
    srv->dispatch_data_message(
        (std::string("/") + dev + ".metronome/ap*/metronome/bypass").c_str(),
        msg);
    loarg[0]->f = bpb;
    srv->dispatch_data_message(
        (std::string("/") + dev + ".metronome/ap*/metronome/bpb").c_str(), msg);
    loarg[0]->f = level;
    srv->dispatch_data_message(
        (std::string("/") + dev + ".metronome/ap*/metronome/a1").c_str(), msg);
    srv->dispatch_data_message(
        (std::string("/") + dev + ".metronome/ap*/metronome/ao").c_str(), msg);
    lo_message_free(msg);
  }
}

bool file_exists(const std::string& fname)
{
  try {
    std::ifstream ifh(fname);
    if(ifh.good())
      return true;
    return false;
  }
  catch(...) {
    return false;
  }
}

TASCAR::pos_t to_tascar(const pos_t& src)
{
  return TASCAR::pos_t(src.x, src.y, src.z);
}

TASCAR::zyx_euler_t to_tascar(const zyx_euler_t& src)
{
  return TASCAR::zyx_euler_t(src.z, src.y, src.x);
}

void sendpinglog(stage_device_id_t cid, double tms, const endpoint_t& ep,
                 void* addr)
{
  if(addr) {
    lo_send((lo_address)addr, "/ping", "id", cid, tms);
    lo_send((lo_address)addr, "/pinga", "iiid", cid, ep.sin_addr.s_addr,
            ep.sin_port, tms);
  }
}

ov_render_tascar_t::ov_render_tascar_t(const std::string& deviceid,
                                       port_t pinglogport_)
    : ov_render_base_t(deviceid), h_jack(NULL), h_webmixer(NULL), tascar(NULL),
      ovboxclient(NULL), pinglogport(pinglogport_), pinglogaddr(nullptr),
      inputports({"system:capture_1", "system:capture_2"}),
      headtrack_tauref(33.315), selfmonitor_delay(0.0)
{
  audiodevice = {"jack", "hw:1", 48000, 96, 2};
  if(pinglogport)
    pinglogaddr =
        lo_address_new("localhost", std::to_string(pinglogport_).c_str());
}

ov_render_tascar_t::~ov_render_tascar_t()
{
  if(is_session_active())
    end_session();
  if(is_audio_active())
    stop_audiobackend();
  if(pinglogaddr)
    lo_address_free(pinglogaddr);
}

void ov_render_tascar_t::create_virtual_acoustics(xmlpp::Element* e_session,
                                                  xmlpp::Element* e_rec,
                                                  xmlpp::Element* e_scene)
{
  stage_device_t& thisdev(stage.stage[stage.thisstagedeviceid]);
  // list of ports on which TASCAR will wait before attempting to connect:
  std::vector<std::string> waitports;
  // b_sender is true if this device is sending audio. If this
  // device is not sending audio, then the stage layout will
  // differ.
  bool b_sender(!thisdev.channels.empty());
  if(b_sender) {
    // set position and orientation of receiver:
    xmlpp::Element* e_pos = e_rec->add_child("position");
    e_pos->add_child_text(
        "0 " + TASCAR::to_string(
                   to_tascar(stage.stage[stage.rendersettings.id].position)));
    xmlpp::Element* e_rot = e_rec->add_child("orientation");
    e_rot->add_child_text(
        "0 " + TASCAR::to_string(to_tascar(
                   stage.stage[stage.rendersettings.id].orientation)));
  }
  // the stage is not empty, which means we are on a stage.  width of
  // stage in degree, for listener-only devices (for sending devices
  // we take positions transferred from database server):
  double stagewidth(160);
  double az(-0.5 * stagewidth);
  double daz(stagewidth / (stage.stage.size() - (!b_sender)) * (M_PI / 180.0));
  az = az * (M_PI / 180.0) - 0.5 * daz;
  double radius(1.2);
  // create sound sources:
  for(auto stagemember : stage.stage) {
    if(stagemember.second.channels.size()) {
      // create sound source only for sending devices:
      if(b_sender || (stagemember.second.id != thisdev.id)) {
        TASCAR::pos_t pos(to_tascar(stagemember.second.position));
        TASCAR::zyx_euler_t rot(to_tascar(stagemember.second.orientation));
        if(!b_sender) {
          // this device is not sending, overwrite stage layout:
          az += daz;
          pos.x = radius * cos(az);
          pos.y = -radius * sin(az);
          pos.z = 0;
          rot.z = (180 / M_PI * (-az + M_PI));
          rot.y = 0;
          rot.x = 0;
        }
        // create a sound source for each device on stage:
        xmlpp::Element* e_src(e_scene->add_child("source"));
        if(stagemember.second.id == thisdev.id) {
          // in case of self-monitoring, this source is called "ego":
          e_src->set_attribute("name", "ego");
        } else {
          e_src->set_attribute("name",
                               get_stagedev_name(stagemember.second.id));
        }
        e_src->set_attribute("dlocation", to_string(pos));
        xmlpp::Element* e_rot = e_src->add_child("orientation");
        e_rot->add_child_text("0 " + TASCAR::to_string(rot));
        // e_src->set_attribute("dorientation", to_string(rot));
        uint32_t kch(0);
        for(auto ch : stagemember.second.channels) {
          // create a sound for each channel:
          ++kch;
          xmlpp::Element* e_snd(e_src->add_child("sound"));
          e_snd->set_attribute("maxdist", "50");
          e_snd->set_attribute("gainmodel", "1");
          e_snd->set_attribute("delayline", "false");
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
          TASCAR::pos_t chpos(to_tascar(ch.position));
          e_snd->set_attribute("x", TASCAR::to_string(chpos.x));
          e_snd->set_attribute("y", TASCAR::to_string(chpos.y));
          e_snd->set_attribute("z", TASCAR::to_string(chpos.z));
          if(ch.directivity == "omni")
            e_snd->set_attribute("type", "omni");
          if(ch.directivity == "cardioid")
            e_snd->set_attribute("type", "cardioidmod");
          if((stagemember.second.id == thisdev.id) &&
             (selfmonitor_delay > 0.0)) {
            xmlpp::Element* e_plugs(e_snd->add_child("plugins"));
            xmlpp::Element* e_delay(e_plugs->add_child("delay"));
            e_delay->set_attribute(
                "delay", TASCAR::to_string(selfmonitor_delay * 0.001));
          }
        }
      }
    }
  }
  if(stage.rendersettings.renderism) {
    // create shoebox room:
    xmlpp::Element* e_rvb(e_scene->add_child("facegroup"));
    e_rvb->set_attribute("name", "room");
    e_rvb->set_attribute(
        "shoebox", TASCAR::to_string(to_tascar(stage.rendersettings.roomsize)));
    e_rvb->set_attribute(
        "reflectivity",
        TASCAR::to_string(1.0 - stage.rendersettings.absorption));
    e_rvb->set_attribute("damping",
                         TASCAR::to_string(stage.rendersettings.damping));
  }
  if(stage.rendersettings.renderreverb) {
    // create reverb engine:
    xmlpp::Element* e_rvb(e_scene->add_child("reverb"));
    e_rvb->set_attribute("type", "simplefdn");
    e_rvb->set_attribute("volumetric", TASCAR::to_string(to_tascar(
                                           stage.rendersettings.roomsize)));
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
  // ambient sounds:
  if(stage.rendersettings.ambientsound.size()) {
    std::string hashname(url2localfilename(stage.rendersettings.ambientsound));
    // test if file exists:
    std::ifstream ambif(hashname);
    if(ambif.good()) {
      xmlpp::Element* e_diff(e_scene->add_child("diffuse"));
      e_diff->set_attribute("name", "ambient");
      e_diff->set_attribute(
          "size", TASCAR::to_string(to_tascar(stage.rendersettings.roomsize)));
      xmlpp::Element* e_plug(e_diff->add_child("plugins"));
      xmlpp::Element* e_snd(e_plug->add_child("sndfile"));
      e_snd->set_attribute("name", hashname);
      e_snd->set_attribute(
          "level", TASCAR::to_string(stage.rendersettings.ambientlevel));
      e_snd->set_attribute("loop", "0");
      e_snd->set_attribute("transport", "false");
      e_snd->set_attribute("license", "CC0");
      e_snd->set_attribute("resample", "true");
    }
  }
  // configure extra modules:
  xmlpp::Element* e_mods(e_session->add_child("modules"));
  xmlpp::Element* e_jackrec = e_mods->add_child("jackrec");
  e_jackrec->set_attribute("url", "osc.udp://localhost:9000/");
  e_mods->add_child("touchosc");
  // create zita-n2j receivers:
  for(auto stagemember : stage.stage) {
    // only create a network receiver when the stage member is sending audio:
    if(stagemember.second.channels.size()) {
      std::string chanlist;
      for(uint32_t k = 0; k < stagemember.second.channels.size(); ++k) {
        if(k)
          chanlist += ",";
        chanlist += std::to_string(k + 1);
      }
      // do not create a network receiver for local device:
      if(stage.thisstagedeviceid != stagemember.second.id) {
        std::string clientname(get_stagedev_name(stagemember.second.id));
        xmlpp::Element* e_sys = e_mods->add_child("system");
        double buff(thisdev.receiverjitter + stagemember.second.senderjitter);
        // provide access to path!
        e_sys->set_attribute(
            "command", "zita-n2j --chan " + chanlist + " --jname " +
                           clientname + "." + stage.thisdeviceid + " --buf " +
                           TASCAR::to_string(buff) + " 0.0.0.0 " +
                           TASCAR::to_string(4464 + 2 * stagemember.second.id));
        e_sys->set_attribute("onunload", "killall zita-n2j");
        // create connections:
        for(size_t c = 0; c < stagemember.second.channels.size(); ++c) {
          if(stage.thisstagedeviceid != stagemember.second.id) {
            std::string srcport(clientname + "." + stage.thisdeviceid +
                                ":out_" + std::to_string(c + 1));
            std::string destport("render." + stage.thisdeviceid + ":" +
                                 clientname + "." + std::to_string(c) + ".0");
            waitports.push_back(srcport);
            xmlpp::Element* e_port = e_session->add_child("connect");
            e_port->set_attribute("src", srcport);
            e_port->set_attribute("dest", destport);
          }
        }
        if(stage.rendersettings.secrec > 0) {
          // create a secondary network receiver with additional jitter buffer:
          if(stage.thisstagedeviceid != stagemember.second.id) {
            std::string clientname(get_stagedev_name(stagemember.second.id) +
                                   "_sec");
            std::string netclientname(
                "n2j_" + std::to_string(stagemember.second.id) + "_sec");
            xmlpp::Element* e_sys = e_mods->add_child("system");
            double buff(thisdev.receiverjitter +
                        stagemember.second.senderjitter);
            e_sys->set_attribute(
                "command",
                "zita-n2j --chan " + chanlist + " --jname " + netclientname +
                    "." + stage.thisdeviceid + " --buf " +
                    TASCAR::to_string(stage.rendersettings.secrec + buff) +
                    " 0.0.0.0 " +
                    TASCAR::to_string(4464 + 2 * stagemember.second.id + 100));
            e_sys->set_attribute("onunload", "killall zita-n2j");
            // create also a route with correct gain settings:
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
  }
  // when a second network receiver is used then also create a bus
  // with a delayed version of the self monitor:
  if((stage.rendersettings.secrec > 0) && (thisdev.channels.size() > 0)) {
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
    // create metronome:
    xmlpp::Element* e_routemetro = e_mods->add_child("route");
    e_routemetro->set_attribute("name", stage.thisdeviceid + ".metronome");
    e_routemetro->set_attribute("channels", "2");
    xmlpp::Element* e_mplug(e_routemetro->add_child("plugins"));
    metronome.set_xmlattr(e_mplug->add_child("metronome"),
                          e_mplug->add_child("delay"));
    // create network sender:
    xmlpp::Element* e_sys = e_mods->add_child("system");
    e_sys->set_attribute(
        "command", "zita-j2n --chan " +
                       std::to_string(thisdev.channels.size()) + " --jname " +
                       stage.thisdeviceid + "_sender --16bit 127.0.0.1 " +
                       std::to_string(4464 + 2 * stage.thisstagedeviceid));
    e_sys->set_attribute("onunload", "killall zita-j2n");
    int chn(0);
    for(auto ch : thisdev.channels) {
      ++chn;
      xmlpp::Element* e_port = e_session->add_child("connect");
      e_port->set_attribute("src", ch.sourceport);
      e_port->set_attribute("dest", stage.thisdeviceid + "_sender:in_" +
                                        std::to_string(chn));
      xmlpp::Element* e_port_m2s = e_session->add_child("connect");
      e_port_m2s->set_attribute("src", stage.thisdeviceid + ".metronome:out.0");
      e_port_m2s->set_attribute("dest", stage.thisdeviceid + "_sender:in_" +
                                            std::to_string(chn));
      xmlpp::Element* e_port_m2e = e_session->add_child("connect");
      e_port_m2e->set_attribute("src", stage.thisdeviceid + ".metronome:out.1");
      e_port_m2e->set_attribute("dest", "render." + stage.thisdeviceid +
                                            ":ego." + std::to_string(chn - 1) +
                                            ".0");
      waitports.push_back(stage.thisdeviceid + "_sender:in_" +
                          std::to_string(chn));
    }
  }
  xmlpp::Element* e_wait = e_mods->add_child("waitforjackport");
  for(auto port : waitports) {
    xmlpp::Element* e_p = e_wait->add_child("port");
    e_p->add_child_text(port);
  }
  // head tracking:
  if(stage.rendersettings.headtracking) {
    xmlpp::Element* e_head = e_mods->add_child("ovheadtracker");
    if(stage.rendersettings.headtrackingport > 0)
      e_head->set_attribute(
          "url", "osc.udp://localhost:" +
                     std::to_string(stage.rendersettings.headtrackingport) +
                     "/");
    std::vector<std::string> actor;
    if(stage.rendersettings.headtrackingrotrec)
      actor.push_back("/" + stage.thisdeviceid + "/master");
    if(stage.rendersettings.headtrackingrotsrc) {
      actor.push_back("/" + stage.thisdeviceid + "/ego");
      e_head->set_attribute("roturl", "osc.udp://localhost:9870/");
      e_head->set_attribute("rotpath", "/*/" + get_stagedev_name(thisdev.id) +
                                           "/zyxeuler");
    }
    e_head->set_attribute("actor", TASCAR::vecstr2str(actor));
    e_head->set_attribute(
        "autoref", std::to_string(1.0 - exp(-1.0 / (30.0 * headtrack_tauref))));
    e_head->set_attribute("levelpattern", "/*/ego/*");
    e_head->set_attribute("name", stage.thisdeviceid);
    e_head->set_attribute("send_only_quaternion", "true");
    if(stage.rendersettings.headtrackingrotrec ||
       stage.rendersettings.headtrackingrotsrc)
      e_head->set_attribute("apply_rot", "true");
    else
      e_head->set_attribute("apply_rot", "false");
    e_head->set_attribute("apply_loc", "false");
  }
}

void ov_render_tascar_t::create_raw_dev(xmlpp::Element* e_session)
{
  stage_device_t& thisdev(stage.stage[stage.thisstagedeviceid]);
  // list of ports on which TASCAR will wait before attempting to connect:
  std::vector<std::string> waitports;
  // configure extra modules:
  xmlpp::Element* e_mods(e_session->add_child("modules"));
  xmlpp::Element* e_jackrec = e_mods->add_child("jackrec");
  e_jackrec->set_attribute("url", "osc.udp://localhost:9000/");
  e_mods->add_child("touchosc");
  //
  uint32_t chcnt(0);
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
                         "." + stage.thisdeviceid + " --buf " +
                         TASCAR::to_string(buff) + " 0.0.0.0 " +
                         TASCAR::to_string(4464 + 2 * stagemember.second.id));
      e_sys->set_attribute("onunload", "killall zita-n2j");
      for(size_t c = 0; c < stagemember.second.channels.size(); ++c) {
        ++chcnt;
        if(stage.thisstagedeviceid != stagemember.second.id) {
          std::string srcport(clientname + "." + stage.thisdeviceid + ":out_" +
                              std::to_string(c + 1));
          std::string destport;
          if(chcnt & 1)
            destport = stage.rendersettings.outputport1;
          else
            destport = stage.rendersettings.outputport2;
          if(!destport.empty()) {
            waitports.push_back(srcport);
            xmlpp::Element* e_port = e_session->add_child("connect");
            e_port->set_attribute("src", srcport);
            e_port->set_attribute("dest", destport);
          }
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
                  "." + stage.thisdeviceid + " --buf " +
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
      e_port->set_attribute("dest", stage.thisdeviceid + "_sender:in_" +
                                        std::to_string(chn));
      waitports.push_back(stage.thisdeviceid + "_sender:in_" +
                          std::to_string(chn));
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
        std::string srcport("master_l");
        if(stage.rendersettings.rectype == "itu51")
          srcport = "master.0L";
        if(stage.rendersettings.rectype == "omni")
          srcport = "master.0";
        e_con->set_attribute("src",
                             "render." + stage.thisdeviceid + ":" + srcport);
        e_con->set_attribute("dest", stage.rendersettings.outputport1);
      }
      if(!stage.rendersettings.outputport2.empty()) {
        xmlpp::Element* e_con(e_session->add_child("connect"));
        std::string srcport("master_r");
        if(stage.rendersettings.rectype == "itu51")
          srcport = "master.1R";
        if(stage.rendersettings.rectype == "omni")
          srcport = "master.0";
        e_con->set_attribute("src",
                             "render." + stage.thisdeviceid + ":" + srcport);
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
        e_snd->set_attribute("delayline", "false");
        e_snd->set_attribute("x", "4");
        xmlpp::Element* e_plugs(e_snd->add_child("plugins"));
        xmlpp::Element* e_sndfile(e_plugs->add_child("sndfile"));
        e_sndfile->set_attribute("name", "announce.flac");
        e_sndfile->set_attribute("level", "57");
        e_sndfile->set_attribute("transport", "false");
        e_sndfile->set_attribute("resample", "true");
        e_sndfile->set_attribute("loop", "0");
        xmlpp::Element* e_mods(e_session->add_child("modules"));
        xmlpp::Element* e_jackrec = e_mods->add_child("jackrec");
        e_jackrec->set_attribute("url", "osc.udp://localhost:9000/");
        e_mods->add_child("touchosc");
      }
    } else {
      if(!stage.host.empty()) {
        create_raw_dev(e_session);
      }
    }
    for(auto xport : stage.rendersettings.xports) {
      xmlpp::Element* e_con(e_session->add_child("connect"));
      e_con->set_attribute("src", xport.first);
      e_con->set_attribute("dest", xport.second);
    }
    if(!stage.host.empty()) {
      // ovboxclient_t rec(desthost, destport, recport, portoffset, prio,
      // secret,
      //                callerid, peer2peer, donotsend, downmixonly);
      ovboxclient = new ovboxclient_t(
          stage.host, stage.port, 4464 + 2 * stage.thisstagedeviceid, 0, 30,
          stage.pin, stage.thisstagedeviceid, stage.rendersettings.peer2peer,
          false, false, stage.stage[stage.thisstagedeviceid].sendlocal);
      if(stage.rendersettings.secrec > 0)
        ovboxclient->add_extraport(100);
      for(auto p : stage.rendersettings.xrecport)
        ovboxclient->add_receiverport(p, p);
      ovboxclient->add_receiverport(9870, 9871);
      if(pinglogaddr)
        ovboxclient->set_ping_callback(sendpinglog, pinglogaddr);
    }
    tsc.doc->write_to_file_formatted("ov-client_debugsession.tsc");
    tascar = new TASCAR::session_t(tsc.doc->write_to_string(),
                                   TASCAR::session_t::LOAD_STRING, "");
    tascar->start();
    // add web mixer tools (node-js server and touchosc interface):
    std::string command;
    std::string ipaddr(ep2ipstr(getipaddr()));
    ipaddr += " '";
    ipaddr += stage.thisdeviceid + " (" + stage.thisdevice.label + ")'";
    if(file_exists("/usr/share/ovclient/webmixer.js")) {
      command = "(cd /usr/share/ovclient/ && node webmixer.js " + ipaddr + ")";
    }
    if(file_exists("webmixer.js")) {
      command = "node webmixer.js " + ipaddr;
    }
    if(!command.empty())
      h_webmixer = new spawn_process_t(command);
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
  if(h_webmixer)
    delete h_webmixer;
  h_webmixer = NULL;
  if(tascar) {
    tascar->stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    delete tascar;
    tascar = NULL;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
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
    if(h_jack)
      delete h_jack;
    std::string devname(audiodevice.devicename);
    if(audiodevice.devicename == "highest") {
      // the device name is not set, use the last one of available
      // devices because this is most likely the one to use (e.g.,
      // external sound card):
      auto devs(list_sound_devices());
      if(!devs.empty())
        devname = devs.rbegin()->dev;
    }
    if(audiodevice.devicename == "plughighest") {
      // the device name is not set, use the last one of available
      // devices because this is most likely the one to use (e.g.,
      // external sound card):
      auto devs(list_sound_devices());
      if(!devs.empty())
        devname = std::string("plug") + devs.rbegin()->dev;
    }
    char cmd[1024];
#ifdef __APPLE__
    sprintf(cmd,
            "JACK_NO_AUDIO_RESERVATION=1 jackd --sync -P 40 -d coreaudio -d %s "
            "-r %g -p %d -n %d",
            devname.c_str(), audiodevice.srate, audiodevice.periodsize,
            audiodevice.numperiods);
#else
    sprintf(cmd,
            "JACK_NO_AUDIO_RESERVATION=1 jackd --sync -P 40 -d alsa -d %s "
            "-r %g -p %d -n %d",
            devname.c_str(), audiodevice.srate, audiodevice.periodsize,
            audiodevice.numperiods);
#endif
    h_jack = new spawn_process_t(cmd);
    // replace sleep by testing for jack presence with timeout:
    sleep(7);
  }
  // get list of input ports:
  jack_client_t* jc;
  jack_options_t opt((jack_options_t)(JackNoStartServer));
  jack_status_t jstat;
  jc = jack_client_open("listchannels", opt, &jstat);
  if(jc) {
    std::vector<std::string> ports;
    const char** pp_ports(
        jack_get_ports(jc, NULL, NULL, JackPortIsOutput | JackPortIsPhysical));
    if(pp_ports) {
      const char** p(pp_ports);
      while(*p) {
        ports.push_back(*p);
        ++p;
      }
      jack_free(pp_ports);
    }
    jack_client_close(jc);
    inputports = ports;
  } else {
    inputports = {"system:capture_1", "system:capture_2"};
  }
}

void ov_render_tascar_t::stop_audiobackend()
{
  ov_render_base_t::stop_audiobackend();
  if(h_jack) {
    delete h_jack;
    h_jack = NULL;
    // wait for jack to clean up properly:
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

void ov_render_tascar_t::add_stage_device(const stage_device_t& stagedevice)
{
  // compare with current stage:
  auto p_stage(stage.stage);
  ov_render_base_t::add_stage_device(stagedevice);
  if((p_stage != stage.stage) && is_session_active()) {
    end_session();
    start_session();
  }
}

void ov_render_tascar_t::rm_stage_device(stage_device_id_t stagedeviceid)
{
  // compare with current stage:
  auto p_stage(stage.stage);
  ov_render_base_t::rm_stage_device(stagedeviceid);
  if((p_stage != stage.stage) && is_session_active()) {
    end_session();
    start_session();
  }
}

void ov_render_tascar_t::set_stage(
    const std::map<stage_device_id_t, stage_device_t>& s)
{
  // compare with current stage:
  auto p_stage(stage.stage);
  ov_render_base_t::set_stage(s);
  if((p_stage != stage.stage) && is_session_active()) {
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
    const render_settings_t& rendersettings,
    stage_device_id_t thisstagedeviceid)
{
  if((rendersettings != stage.rendersettings) ||
     (thisstagedeviceid != stage.thisstagedeviceid)) {
    ov_render_base_t::set_render_settings(rendersettings, thisstagedeviceid);
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

void ov_render_tascar_t::getbitrate(double& txrate, double& rxrate)
{
  if(ovboxclient)
    ovboxclient->getbitrate(txrate, rxrate);
}

std::vector<std::string> ov_render_tascar_t::get_input_channel_ids() const
{
  return inputports;
}

double ov_render_tascar_t::get_load() const
{
  if(tascar) {
    return 0.01 * tascar->get_cpu_load();
  }
  return 0;
}

void ov_render_tascar_t::set_extra_config(const std::string& js)
{
  try {
    if(!js.empty()) {
      nlohmann::json xcfg(nlohmann::json::parse(js));
      if(xcfg["headtrack"].is_object() &&
         !xcfg["headtrack"]["tauref"].is_null())
        headtrack_tauref = xcfg["headtrack"].value("tauref", 33.315);
      if(xcfg["monitor"].is_object() && !xcfg["monitor"]["delay"].is_null())
        selfmonitor_delay = xcfg["monitor"].value("delay", 0.0);
      if(xcfg["metronome"].is_object()) {
        metronome_t newmetro(xcfg["metronome"]);
        if(newmetro != metronome) {
          bool delaychanged(metronome.delay != newmetro.delay);
          metronome = newmetro;
          if(is_session_active()) {
            if(delaychanged) {
              // workaround, no OSC control of delay yet:
              end_session();
              start_session();
            } else {
              metronome.update_osc(tascar, stage.thisdeviceid);
            }
          }
        }
      }
    }
  }
  catch(const std::exception& e) {
    throw TASCAR::ErrMsg(std::string("set_extra_config: ") + e.what());
  }
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
