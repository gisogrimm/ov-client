#include "ov_client_orlandoviols.h"
#include "RSJparser.tcc"
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

stage_device_t get_stage_dev(RSJresource& dev)
{
  stage_device_t stagedev;
  stagedev.id = dev["id"].as<int>(-1);
  stagedev.label = dev["label"].as<std::string>("");
  for(auto ch : dev["channels"].as_array()) {
    device_channel_t devchannel;
    devchannel.sourceport = ch["sourceport"].as<std::string>("");
    devchannel.gain = ch["gain"].as<double>(1.0);
    devchannel.position.x = ch["position"]["x"].as<double>(0);
    devchannel.position.y = ch["position"]["y"].as<double>(0);
    devchannel.position.z = ch["position"]["z"].as<double>(0);
    stagedev.channels.push_back(devchannel);
  }
  /// Position of the stage device in the virtual space:
  stagedev.position.x = dev["position"]["x"].as<double>(0);
  stagedev.position.y = dev["position"]["y"].as<double>(0);
  stagedev.position.z = dev["position"]["z"].as<double>(0);
  /// Orientation of the stage device in the virtual space, ZYX Euler angles:
  stagedev.orientation.z = dev["orientation"]["z"].as<double>(0);
  stagedev.orientation.y = dev["orientation"]["y"].as<double>(0);
  stagedev.orientation.x = dev["orientation"]["x"].as<double>(0);
  /// Linear gain of the stage device:
  stagedev.gain = dev["gain"].as<double>(1.0);
  /// Mute flag:
  stagedev.mute = dev["mute"].as<bool>(false);
  stagedev.receiverjitter = dev["jitter"]["receive"].as<double>(5);
  stagedev.senderjitter = dev["jitter"]["send"].as<double>(5);
  /// send to local IP address if in same network?
  stagedev.sendlocal = dev["sendlocal"].as<bool>(true);
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
        RSJresource js_stagecfg(stagecfg);
        if(js_stagecfg["firmwareupdate"].as<bool>(false)) {
          std::ofstream ofh("ov-client.firmwareupdate");
          quitrequest_ = true;
        } else {
          RSJresource js_audio(js_stagecfg["audiocfg"]);
          audio_device_t audio;
          backend.clear_stage();
          audio.drivername = js_audio["driver"].as<std::string>("jack");
          audio.devicename = js_audio["device"].as<std::string>("hw:1");
          audio.srate = js_audio["srate"].as<double>(48000);
          audio.periodsize = js_audio["periodsize"].as<int>(96);
          audio.numperiods = js_audio["numperiods"].as<int>(2);
          backend.configure_audio_backend(audio);
          if(js_audio["restart"].as<bool>(false)) {
            backend.stop_audiobackend();
            backend.start_audiobackend();
          }
          RSJresource js_rendersettings(js_stagecfg["rendersettings"]);
          backend.set_thisdev(get_stage_dev(js_rendersettings));
          RSJresource js_stage(js_stagecfg["room"]);
          std::string stagehost(js_stage["host"].as<std::string>(""));
          port_t stageport(js_stage["port"].as<int>(0));
          secret_t stagepin(js_stage["pin"].as<secret_t>(0));
          backend.set_relay_server(stagehost, stageport, stagepin);
          RSJresource js_roomsize(js_stage["size"]);
          RSJresource js_reverb(js_stage["reverb"]);
          render_settings_t rendersettings;
          rendersettings.id = js_rendersettings["id"].as<int>(-1);
          rendersettings.roomsize.x = js_roomsize["x"].as<double>(0);
          rendersettings.roomsize.y = js_roomsize["y"].as<double>(0);
          rendersettings.roomsize.z = js_roomsize["z"].as<double>(0);
          rendersettings.absorption = js_reverb["absorption"].as<double>(0.6);
          rendersettings.damping = js_reverb["damping"].as<double>(0.7);
          rendersettings.reverbgain = js_reverb["gain"].as<double>(0.4);
          rendersettings.renderreverb =
              js_rendersettings["renderreverb"].as<bool>(true);
          rendersettings.renderism =
              js_rendersettings["renderism"].as<bool>(false);
          rendersettings.outputport1 =
              js_rendersettings["outputport1"].as<std::string>("");
          rendersettings.outputport2 =
              js_rendersettings["outputport2"].as<std::string>("");
          rendersettings.rawmode = js_rendersettings["rawmode"].as<bool>(false);
          rendersettings.rectype =
              js_rendersettings["rectype"].as<std::string>("ortf");
          rendersettings.secrec = js_rendersettings["secrec"].as<double>(0);
          rendersettings.egogain = js_rendersettings["egogain"].as<double>(1.0);
          rendersettings.peer2peer =
              js_rendersettings["peer2peer"].as<bool>(true);
          // ambient sound:
          rendersettings.ambientsound =
              js_stage["ambientsound"].as<std::string>("");
          rendersettings.ambientlevel = js_stage["ambientlevel"].as<double>(0);
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
          RSJarray js_xports(js_rendersettings["xport"].as_array());
          for(auto xp : js_xports) {
            RSJarray js_xp(xp.as_array());
            if(js_xp.size() == 2) {
              std::string key(js_xp[0].as<std::string>(""));
              if(key.size())
                rendersettings.xports[key] = js_xp[1].as<std::string>("");
            }
          }
          rendersettings.xrecport.clear();
          RSJarray js_xrecports(js_rendersettings["xrecport"].as_array());
          for(auto xrp : js_xrecports) {
            int p(xrp.as<int>(0));
            if(p > 0)
              rendersettings.xrecport.push_back(p);
          }
          rendersettings.headtracking =
              js_rendersettings["headtracking"].as<bool>(false);
          rendersettings.headtrackingrotrec =
              js_rendersettings["headtrackingrot"].as<bool>(true);
          rendersettings.headtrackingrotsrc =
              js_rendersettings["headtrackingrotsrc"].as<bool>(true);
          rendersettings.headtrackingport =
              js_rendersettings["headtrackingport"].as<int>(0);
          backend.set_render_settings(
              rendersettings, js_rendersettings["stagedevid"].as<int>(0));
          RSJarray js_stagedevs(js_stagecfg["roomdev"].as_array());
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
