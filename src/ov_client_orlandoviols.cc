#include "ov_client_orlandoviols.h"
#include "RSJparser.tcc"
#include "errmsg.h"
#include <alsa/asoundlib.h>
#include <curl/curl.h>

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
    : ov_client_base_t(backend), runservice(true), lobby(lobby)
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

struct snddevname_t {
  std::string dev;
  std::string desc;
};

std::vector<snddevname_t> listdev()
{
  std::vector<snddevname_t> retv;
  char** hints;
  int err;
  char** n;
  char* name;
  char* desc;

  /* Enumerate sound devices */
  err = snd_device_name_hint(-1, "pcm", (void***)&hints);
  if(err != 0) {
    return retv;
  }
  n = hints;
  while(*n != NULL) {
    name = snd_device_name_get_hint(*n, "NAME");
    desc = snd_device_name_get_hint(*n, "DESC");
    if(strncmp("hw:", name, 3) == 0) {
      snddevname_t dname;
      dname.dev = name;
      dname.desc = desc;
      if(dname.desc.find("\n"))
        dname.desc.erase(dname.desc.find("\n"));
      retv.push_back(dname);
    }
    if(name && strcmp("null", name))
      free(name);
    if(desc && strcmp("null", desc))
      free(desc);
    n++;
  }
  // Free hint buffer too
  snd_device_name_free_hint((void**)hints);
  return retv;
}

std::string ov_client_orlandoviols_t::get_device_init(std::string url,
                                                      const std::string& device,
                                                      std::string& hash)
{
  std::vector<snddevname_t> alsadevs(listdev());
  std::string jsdevs("{");
  for(auto d : alsadevs)
    jsdevs += "\"" + d.dev + "\":\"" + d.desc + "\",";
  if(jsdevs.size())
    jsdevs.erase(jsdevs.end() - 1);
  jsdevs += "}";
  std::cout << jsdevs << std::endl;
  CURLcode res;
  std::string retv;
  struct webCURL::MemoryStruct chunk;
  chunk.memory =
      (char*)malloc(1); /* will be grown as needed by the realloc above */
  chunk.size = 0;       /* no data at this point */

  url += "?ovclient=" + device;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
  /* send all data to this function  */
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCURL::WriteMemoryCallback);
  /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);

  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsdevs.c_str());

  /* get it! */
  res = curl_easy_perform(curl);

  /* check for errors */
  if(res == CURLE_OK) {
    retv.insert(0, chunk.memory, chunk.size);
    // printf("%lu bytes retrieved\n", (unsigned long)chunk.size);
  }
  free(chunk.memory);
  return retv;
}

void ov_client_orlandoviols_t::service()
{
  std::string hash;
  double gracetime(8.0);
  while(runservice) {
    std::string stagecfg(get_device_init(lobby, backend.get_deviceid(), hash));
    if(!stagecfg.empty()) {
      RSJresource my_json(stagecfg);
      std::cout << "-----------------------------------------------"
                << std::endl;
      std::cout << stagecfg << std::endl;
      // jacksettings_t jacks;
      // jacks.device = my_json["jackdevice"].as<std::string>("hw:1");
      // jacks.rate = my_json["jackrate"].as<int>(48000);
      // jacks.period = my_json["jackperiod"].as<int>(96);
      // jacks.buffers = my_json["jackbuffers"].as<int>(2);
      // return jacks;
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
