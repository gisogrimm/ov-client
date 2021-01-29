#include "ov_client_orlandoviols.h"
#include "errmsg.h"
#include "ov_tools.h"
#include "soundcardtools.h"
#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <string.h>
#include <udpsocket.h>
#include <unistd.h>

CURL* curl;

namespace webCURL {

  struct MemoryStruct {
    char* memory;
    size_t size;
  };

  static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb,
                                    void* userp)
  {
    size_t realsize = size * nmemb;
    struct MemoryStruct* mem = (struct MemoryStruct*)userp;
    char* ptr = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
      /* out of memory! */
      printf("not enough memory (realloc returned NULL)\n");
      return 0;
    }
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
  }

} // namespace webCURL

ov_client_orlandoviols_t::ov_client_orlandoviols_t(ov_render_base_t& backend,
                                                   const std::string& lobby)
    : ov_client_base_t(backend), runservice(true), lobby(lobby),
      quitrequest_(false), isovbox(is_ovbox())
{
  // curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if(!curl)
    throw ErrMsg("Unable to initialize curl");
}

void ov_client_orlandoviols_t::start_service()
{
  runservice = true;
  servicethread = std::thread(&ov_client_orlandoviols_t::service, this);
}

void ov_client_orlandoviols_t::stop_service()
{
  runservice = false;
  servicethread.join();
}

void ov_client_orlandoviols_t::report_error(std::string url,
                                            const std::string& device,
                                            const std::string& msg)
{
  std::string retv;
  struct webCURL::MemoryStruct chunk;
  chunk.memory =
      (char*)malloc(1); /* will be grown as needed by the realloc above */
  chunk.size = 0;       /* no data at this point */
  url += "?ovclientmsg=" + device;
  curl_easy_reset(curl);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCURL::WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, msg.c_str());
  curl_easy_perform(curl);
  free(chunk.memory);
}

bool ov_client_orlandoviols_t::download_file(const std::string& url,
                                             const std::string& dest)
{
  CURLcode res;
  std::string retv;
  struct webCURL::MemoryStruct chunk;
  chunk.memory =
      (char*)malloc(1); /* will be grown as needed by the realloc above */
  chunk.size = 0;       /* no data at this point */
  curl_easy_reset(curl);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCURL::WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  res = curl_easy_perform(curl);
  if(res == CURLE_OK) {
    std::ofstream ofh(dest);
    ofh.write(chunk.memory, chunk.size);
    free(chunk.memory);
    return true;
  }
  free(chunk.memory);
  return false;
}

std::string ov_client_orlandoviols_t::device_update(std::string url,
                                                    const std::string& device,
                                                    std::string& hash)
{
  char chost[1024];
  memset(chost, 0, 1024);
  std::string hostname;
  if(0 == gethostname(chost, 1023))
    hostname = chost;
  std::string jsinchannels("{");
  uint32_t nch(0);
  for(auto ch : backend.get_input_channel_ids()) {
    jsinchannels += "\"" + std::to_string(nch) + "\":\"" + ch + "\",";
    ++nch;
  }
  if(jsinchannels.size())
    jsinchannels.erase(jsinchannels.size() - 1, 1);
  jsinchannels += "}";
  std::vector<snddevname_t> alsadevs(list_sound_devices());
  std::string jsdevs("{");
  for(auto d : alsadevs)
    jsdevs += "\"" + d.dev + "\":\"" + d.desc + "\",";
  if((jsdevs.size() > 0) && (alsadevs.size() > 0))
    jsdevs.erase(jsdevs.size() - 1, 1);
  jsdevs += "}";
  // std::cout << jsdevs << std::endl;
  double txrate(0);
  double rxrate(0);
  backend.getbitrate(txrate, rxrate);
  std::string jsmsg("{");
  jsmsg += "\"alsadevs\":" + jsdevs + ",";
  jsmsg += "\"hwinputchannels\":" + jsinchannels + ",";
  jsmsg += "\"cpuload\":" + std::to_string(backend.get_load()) + ",";
  jsmsg += "\"bandwidth\":{\"tx\":\"" + std::to_string(txrate) +
           "\",\"rx\":\"" + std::to_string(rxrate) + "\"},";
  jsmsg += "\"localip\":\"" + ep2ipstr(getipaddr()) + "\",";
  jsmsg += "\"version\":\"ovclient-" + std::string(OVBOXVERSION) + "\",";
  jsmsg += "\"isovbox\":" + std::string(isovbox ? "true" : "false");
  jsmsg += "}";
  CURLcode res;
  std::string retv;
  struct webCURL::MemoryStruct chunk;
  chunk.memory =
      (char*)malloc(1); /* will be grown as needed by the realloc above */
  chunk.size = 0;       /* no data at this point */
  url += "?ovclient2=" + device + "&hash=" + hash;
  if(hostname.size() > 0)
    url += "&host=" + hostname;
  curl_easy_reset(curl);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCURL::WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsmsg.c_str());
  res = curl_easy_perform(curl);
  if(res == CURLE_OK)
    retv.insert(0, chunk.memory, chunk.size);
  free(chunk.memory);
  std::stringstream ss(retv);
  std::string to;
  bool first(true);
  retv = "";
  while(std::getline(ss, to, '\n')) {
    if(first)
      hash = to;
    else
      retv += to + '\n';
    first = false;
  }
  if(retv.size())
    retv.erase(retv.size() - 1, 1);
  return retv;
}

void ov_client_orlandoviols_t::register_device(std::string url,
                                               const std::string& device)
{
  struct webCURL::MemoryStruct chunk;
  chunk.memory =
      (char*)malloc(1); /* will be grown as needed by the realloc above */
  chunk.size = 0;       /* no data at this point */
  url += "?setver=" + device + "&ver=ovclient-" + OVBOXVERSION;
  curl_easy_reset(curl);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCURL::WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  curl_easy_perform(curl);
  free(chunk.memory);
}

stage_device_t get_stage_dev(nlohmann::json& dev)
{
  stage_device_t stagedev;
  stagedev.id = dev.value("id", -1);
  stagedev.label = dev.value("label", "");
  nlohmann::json channels(dev["channels"]);
  if(channels.is_array())
    for(auto ch : channels) {
      device_channel_t devchannel;
      devchannel.sourceport = ch.value("sourceport", "");
      devchannel.gain = ch.value("gain", 1.0);
      nlohmann::json chpos(ch["position"]);
      if(chpos.is_object()) {
        devchannel.position.x = chpos.value("x", 0.0);
        devchannel.position.y = chpos.value("y", 0.0);
        devchannel.position.z = chpos.value("z", 0.0);
      } else {
        devchannel.position.x = 0.0;
        devchannel.position.y = 0.0;
        devchannel.position.z = 0.0;
      }
      devchannel.directivity = ch.value("directivity", "omni");
      stagedev.channels.push_back(devchannel);
    }
  /// Position of the stage device in the virtual space:
  nlohmann::json devpos(dev["position"]);
  if(devpos.is_object()) {
    stagedev.position.x = devpos.value("x", 0.0);
    stagedev.position.y = devpos.value("y", 0.0);
    stagedev.position.z = devpos.value("z", 0.0);
  } else {
    stagedev.position.x = 0.0;
    stagedev.position.y = 0.0;
    stagedev.position.z = 0.0;
  }
  /// Orientation of the stage device in the virtual space, ZYX Euler angles:
  nlohmann::json devorient(dev["orientation"]);
  if(devorient.is_object()) {
    stagedev.orientation.z = devorient.value("z", 0.0);
    stagedev.orientation.y = devorient.value("y", 0.0);
    stagedev.orientation.x = devorient.value("x", 0.0);
  } else {
    stagedev.orientation.z = 0.0;
    stagedev.orientation.y = 0.0;
    stagedev.orientation.x = 0.0;
  }
  /// Linear gain of the stage device:
  stagedev.gain = dev.value("gain", 1.0);
  /// Mute flag:
  stagedev.mute = dev.value("mute", false);
  // jitter buffer length:
  nlohmann::json jitter(dev["jitter"]);
  if(jitter.is_object()) {
    stagedev.receiverjitter = jitter.value("receive", 5.0);
    stagedev.senderjitter = jitter.value("send", 5.0);
  } else {
    stagedev.receiverjitter = 5.0;
    stagedev.senderjitter = 5.0;
  }
  /// send to local IP address if in same network?
  stagedev.sendlocal = dev.value("sendlocal", true);
  return stagedev;
}

std::string ovstrrep(std::string s, const std::string& pat,
                     const std::string& rep)
{
  std::string out_string("");
  std::string::size_type len = pat.size();
  std::string::size_type pos;
  while((pos = s.find(pat)) < s.size()) {
    out_string += s.substr(0, pos);
    out_string += rep;
    s.erase(0, pos + len);
  }
  s = out_string + s;
  return s;
}

void ov_client_orlandoviols_t::service()
{
  try {
    register_device(lobby, backend.get_deviceid());
  }
  catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
  try {
    report_error(lobby, backend.get_deviceid(), "");
  }
  catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
  try {
    download_file(lobby + "/announce.flac", "announce.flac");
  }
  catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
  std::string hash;
  double gracetime(9.0);
  while(runservice) {
    std::string stagecfg(device_update(lobby, backend.get_deviceid(), hash));
    if(!stagecfg.empty()) {
      try {
        report_error(lobby, backend.get_deviceid(), "");
        nlohmann::json js_stagecfg(nlohmann::json::parse(stagecfg));
        if(!js_stagecfg["frontendconfig"].is_null()) {
          std::ofstream ofh("ov-client.cfg");
          ofh << js_stagecfg["frontendconfig"].dump();
          quitrequest_ = true;
        }
        if(js_stagecfg.value("firmwareupdate", false)) {
          std::ofstream ofh("ov-client.firmwareupdate");
          quitrequest_ = true;
        }
        if(!quitrequest_) {
          nlohmann::json js_audio(js_stagecfg["audiocfg"]);
          if(!js_audio.is_null()) {
            audio_device_t audio;
            backend.clear_stage();
            audio.drivername = js_audio.value("driver", "jack");
            audio.devicename = js_audio.value("device", "hw:1");
            audio.srate = js_audio.value("srate", 48000.0);
            audio.periodsize = js_audio.value("periodsize", 96);
            audio.numperiods = js_audio.value("numperiods", 2);
            backend.configure_audio_backend(audio);
            if(js_audio.value("restart", false)) {
              backend.stop_audiobackend();
              backend.start_audiobackend();
            }
          }
          nlohmann::json js_rendersettings(js_stagecfg["rendersettings"]);
          if(!js_rendersettings.is_null())
            backend.set_thisdev(get_stage_dev(js_rendersettings));
          nlohmann::json js_stage(js_stagecfg["room"]);
          std::string stagehost(js_stage.value("host", ""));
          port_t stageport(js_stage.value("port", 0));
          secret_t stagepin(js_stage.value("pin", 0u));
          backend.set_relay_server(stagehost, stageport, stagepin);
          nlohmann::json js_roomsize(js_stage["size"]);
          nlohmann::json js_reverb(js_stage["reverb"]);
          render_settings_t rendersettings;
          rendersettings.id = js_rendersettings.value("id", -1);
          rendersettings.roomsize.x = js_roomsize.value("x", 0.0);
          rendersettings.roomsize.y = js_roomsize.value("y", 0.0);
          rendersettings.roomsize.z = js_roomsize.value("z", 0.0);
          rendersettings.absorption = js_reverb.value("absorption", 0.6);
          rendersettings.damping = js_reverb.value("damping", 0.7);
          rendersettings.reverbgain = js_reverb.value("gain", 0.4);
          rendersettings.renderreverb =
              js_rendersettings.value("renderreverb", true);
          rendersettings.renderism =
              js_rendersettings.value("renderism", false);
          rendersettings.outputport1 =
              js_rendersettings.value("outputport1", "");
          rendersettings.outputport2 =
              js_rendersettings.value("outputport2", "");
          rendersettings.rawmode = js_rendersettings.value("rawmode", false);
          rendersettings.rectype = js_rendersettings.value("rectype", "ortf");
          rendersettings.secrec = js_rendersettings.value("secrec", 0.0);
          rendersettings.egogain = js_rendersettings.value("egogain", 1.0);
          rendersettings.peer2peer = js_rendersettings.value("peer2peer", true);
          // ambient sound:
          rendersettings.ambientsound = js_stage.value("ambientsound", "");
          rendersettings.ambientlevel = js_stage.value("ambientlevel", 0.0);
          rendersettings.ambientsound =
              ovstrrep(rendersettings.ambientsound, "\\/", "/");
          // if not empty then download file to hash code:
          if(rendersettings.ambientsound.size()) {
            std::string hashname(
                url2localfilename(rendersettings.ambientsound));
            // test if file already exists:
            std::ifstream ambif(hashname);
            if(!ambif.good()) {
              if(!download_file(rendersettings.ambientsound, hashname)) {
                report_error(lobby, backend.get_deviceid(),
                             "Unable to download ambient sound file from " +
                                 rendersettings.ambientsound);
              }
            }
          }
          //
          rendersettings.xports.clear();
          nlohmann::json js_xports(js_rendersettings["xport"]);
          if(js_xports.is_array())
            for(auto xp : js_xports) {
              if(xp.is_array())
                if((xp.size() == 2) && xp[0].is_string() && xp[1].is_string()) {
                  std::string key(xp[0].get<std::string>());
                  if(key.size())
                    rendersettings.xports[key] = xp[1].get<std::string>();
                }
            }
          rendersettings.xrecport.clear();
          nlohmann::json js_xrecports(js_rendersettings["xrecport"]);
          if(js_xrecports.is_array())
            for(auto xrp : js_xrecports) {
              if(xrp.is_number()) {
                int p(xrp.get<int>());
                if(p > 0)
                  rendersettings.xrecport.push_back(p);
              }
            }
          rendersettings.headtracking =
              js_rendersettings.value("headtracking", false);
          rendersettings.headtrackingrotrec =
              js_rendersettings.value("headtrackingrot", true);
          rendersettings.headtrackingrotsrc =
              js_rendersettings.value("headtrackingrotsrc", true);
          rendersettings.headtrackingport =
              js_rendersettings.value("headtrackingport", 0);
          backend.set_render_settings(rendersettings,
                                      js_rendersettings.value("stagedevid", 0));
          if(!js_rendersettings["extracfg"].is_null())
            backend.set_extra_config(js_rendersettings["extracfg"].dump());
          nlohmann::json js_stagedevs(js_stagecfg["roomdev"]);
          if(js_stagedevs.is_array())
            for(auto dev : js_stagedevs) {
              backend.add_stage_device(get_stage_dev(dev));
            }
          if(!backend.is_audio_active())
            backend.start_audiobackend();
          if(!backend.is_session_active())
            backend.start_session();
        }
      }
      catch(const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        report_error(lobby, backend.get_deviceid(), e.what());
      }
    }
    double t(0);
    while((t < gracetime) && runservice) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      t += 0.001;
    }
  }
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
