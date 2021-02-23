#include "ov_client_orlandoviols.h"
#include "../tascar/libtascar/src/session.h"
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

bool ov_client_orlandoviols_t::report_error(std::string url,
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
  if(curl_easy_perform(curl) != CURLE_OK) {
    free(chunk.memory);
    return false;
  }
  free(chunk.memory);
  return true;
}

bool ov_client_orlandoviols_t::download_file(const std::string& url,
                                             const std::string& dest)
{
  CURLcode res;
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

/**
   \brief Pull device configuration.
   \param url Server URL for device REST API, e.g.,
   http://oldbox.orlandoviols.com/ \param device Device ID (typically MAC
   address) \retval hash Hash string of previous device configuration, or empty
   string \return Empty string, or string containing json device configuration
 */
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
  if(curl_easy_perform(curl) != CURLE_OK) {
    free(chunk.memory);
    throw TASCAR::ErrMsg("Unable to register device with url \"" + url + "\".");
  }
  std::string result;
  result.insert(0, chunk.memory, chunk.size);
  free(chunk.memory);
  if(result != "OK")
    throw TASCAR::ErrMsg("The front end did not respond \"OK\".");
}

stage_device_t get_stage_dev(nlohmann::json& dev)
{
  try {
    stage_device_t stagedev;
    stagedev.id = my_js_value(dev, "id", -1);
    stagedev.label = my_js_value(dev, "label", std::string(""));
    nlohmann::json channels(dev["channels"]);
    if(channels.is_array())
      for(auto ch : channels) {
        device_channel_t devchannel;
        devchannel.sourceport = my_js_value(ch, "sourceport", std::string(""));
        devchannel.gain = my_js_value(ch, "gain", 1.0);
        nlohmann::json chpos(ch["position"]);
        devchannel.position.x = my_js_value(chpos, "x", 0.0);
        devchannel.position.y = my_js_value(chpos, "y", 0.0);
        devchannel.position.z = my_js_value(chpos, "z", 0.0);
        devchannel.directivity =
            my_js_value(ch, "directivity", std::string("omni"));
        stagedev.channels.push_back(devchannel);
      }
    /// Position of the stage device in the virtual space:
    nlohmann::json devpos(dev["position"]);
    stagedev.position.x = my_js_value(devpos, "x", 0.0);
    stagedev.position.y = my_js_value(devpos, "y", 0.0);
    stagedev.position.z = my_js_value(devpos, "z", 0.0);
    /// Orientation of the stage device in the virtual space, ZYX Euler angles:
    nlohmann::json devorient(dev["orientation"]);
    stagedev.orientation.z = my_js_value(devorient, "z", 0.0);
    stagedev.orientation.y = my_js_value(devorient, "y", 0.0);
    stagedev.orientation.x = my_js_value(devorient, "x", 0.0);
    /// Linear gain of the stage device:
    stagedev.gain = my_js_value(dev, "gain", 1.0);
    /// Mute flag:
    stagedev.mute = my_js_value(dev, "mute", false);
    // jitter buffer length:
    nlohmann::json jitter(dev["jitter"]);
    stagedev.receiverjitter = my_js_value(jitter, "receive", 5.0);
    stagedev.senderjitter = my_js_value(jitter, "send", 5.0);
    /// send to local IP address if in same network?
    stagedev.sendlocal = my_js_value(dev, "sendlocal", true);
    return stagedev;
  }
  catch(const std::exception& e) {
    throw TASCAR::ErrMsg(std::string("get_stage_dev: ") + e.what());
  }
}

// main frontend interface function:
void ov_client_orlandoviols_t::service()
{
  try {
    register_device(lobby, backend.get_deviceid());
    if(!report_error(lobby, backend.get_deviceid(), ""))
      throw TASCAR::ErrMsg("Unable to reset error message.");
    if(!download_file(lobby + "/announce.flac", "announce.flac"))
      throw TASCAR::ErrMsg("Unable to download announcement file from server.");
  }
  catch(const std::exception& e) {
    quitrequest_ = true;
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << "Invalid URL or server may be down." << std::endl;
    return;
  }
  std::string hash;
  double gracetime(9.0);
  while(runservice) {
    try {
      std::string stagecfg(device_update(lobby, backend.get_deviceid(), hash));
      if(!stagecfg.empty()) {
        try {
          nlohmann::json js_stagecfg(nlohmann::json::parse(stagecfg));
          if(!js_stagecfg["frontendconfig"].is_null()) {
            std::ofstream ofh("ov-client.cfg");
            ofh << js_stagecfg["frontendconfig"].dump();
            quitrequest_ = true;
          }
          if(my_js_value(js_stagecfg, "firmwareupdate", false)) {
            std::ofstream ofh("ov-client.firmwareupdate");
            quitrequest_ = true;
          }
          if(my_js_value(js_stagecfg, "usedevversion", false)) {
            std::ofstream ofh("ov-client.devversion");
            quitrequest_ = true;
          }
          if(!quitrequest_) {
            nlohmann::json js_audio(js_stagecfg["audiocfg"]);
            if(!js_audio.is_null()) {
              audio_device_t audio;
              // backend.clear_stage();
              audio.drivername =
                  my_js_value(js_audio, "driver", std::string("jack"));
              audio.devicename =
                  my_js_value(js_audio, "device", std::string("hw:1"));
              audio.srate = my_js_value(js_audio, "srate", 48000.0);
              audio.periodsize = my_js_value(js_audio, "periodsize", 96);
              audio.numperiods = my_js_value(js_audio, "numperiods", 2);
              backend.configure_audio_backend(audio);
              if(my_js_value(js_audio, "restart", false)) {
                backend.stop_audiobackend();
                backend.start_audiobackend();
              }
            }
            nlohmann::json js_rendersettings(js_stagecfg["rendersettings"]);
            if(!js_rendersettings.is_null())
              backend.set_thisdev(get_stage_dev(js_rendersettings));
            nlohmann::json js_stage(js_stagecfg["room"]);
            if(js_stage.is_object()) {
              std::string stagehost(
                  my_js_value(js_stage, "host", std::string("")));
              port_t stageport(my_js_value(js_stage, "port", 0));
              secret_t stagepin(my_js_value(js_stage, "pin", 0u));
              backend.set_relay_server(stagehost, stageport, stagepin);
              nlohmann::json js_roomsize(js_stage["size"]);
              nlohmann::json js_reverb(js_stage["reverb"]);
              render_settings_t rendersettings;
              rendersettings.id = my_js_value(js_rendersettings, "id", -1);
              rendersettings.roomsize.x = my_js_value(js_roomsize, "x", 25.0);
              rendersettings.roomsize.y = my_js_value(js_roomsize, "y", 13.0);
              rendersettings.roomsize.z = my_js_value(js_roomsize, "z", 7.5);
              rendersettings.absorption =
                  my_js_value(js_reverb, "absorption", 0.6);
              rendersettings.damping = my_js_value(js_reverb, "damping", 0.7);
              rendersettings.reverbgain = my_js_value(js_reverb, "gain", 0.4);
              rendersettings.renderreverb =
                  my_js_value(js_rendersettings, "renderreverb", true);
              rendersettings.renderism =
                  my_js_value(js_rendersettings, "renderism", false);
              rendersettings.outputport1 = my_js_value(
                  js_rendersettings, "outputport1", std::string(""));
              rendersettings.outputport2 = my_js_value(
                  js_rendersettings, "outputport2", std::string(""));
              rendersettings.rawmode =
                  my_js_value(js_rendersettings, "rawmode", false);
              rendersettings.rectype = my_js_value(js_rendersettings, "rectype",
                                                   std::string("ortf"));
              rendersettings.secrec =
                  my_js_value(js_rendersettings, "secrec", 0.0);
              rendersettings.egogain =
                  my_js_value(js_rendersettings, "egogain", 1.0);
              rendersettings.peer2peer =
                  my_js_value(js_rendersettings, "peer2peer", true);
              // ambient sound:
              rendersettings.ambientsound =
                  my_js_value(js_stage, "ambientsound", std::string(""));
              rendersettings.ambientlevel =
                  my_js_value(js_stage, "ambientlevel", 0.0);
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
                    if((xp.size() == 2) && xp[0].is_string() &&
                       xp[1].is_string()) {
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
                  my_js_value(js_rendersettings, "headtracking", false);
              rendersettings.headtrackingrotrec =
                  my_js_value(js_rendersettings, "headtrackingrot", true);
              rendersettings.headtrackingrotsrc =
                  my_js_value(js_rendersettings, "headtrackingrotsrc", true);
              rendersettings.headtrackingport =
                  my_js_value(js_rendersettings, "headtrackingport", 0);
              backend.set_render_settings(
                  rendersettings,
                  my_js_value(js_rendersettings, "stagedevid", 0));
              if(!js_rendersettings["extracfg"].is_null())
                backend.set_extra_config(js_rendersettings["extracfg"].dump());
              nlohmann::json js_stagedevs(js_stagecfg["roomdev"]);
              std::map<stage_device_id_t, stage_device_t> newstage;
              if(js_stagedevs.is_array()) {
                for(auto dev : js_stagedevs) {
                  auto sdev(get_stage_dev(dev));
                  newstage[sdev.id] = sdev;
                }
                backend.set_stage(newstage);
              }
            }
            if(!backend.is_audio_active())
              backend.start_audiobackend();
            if(!backend.is_session_active())
              backend.start_session();
          }
          report_error(lobby, backend.get_deviceid(), "");
        }
        catch(const std::exception& e) {
          std::cerr << "Error: " << e.what() << std::endl;
          report_error(lobby, backend.get_deviceid(), e.what());
          DEBUG(stagecfg);
        }
      }
      double t(0);
      while((t < gracetime) && runservice) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        t += 0.001;
      }
    }
    catch(const std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      std::cerr << "retrying in 5 seconds" << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(5));
    }
  }
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
