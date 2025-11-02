/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2025 Giso Grimm
 */
/*
 * ov-client is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * ov-client is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with ov-client. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../libov/tascar/libtascar/include/tascar_os.h"
#include "../libov/tascar/libtascar/include/tictoctimer.h"
#include "ov_tools.h"
#include <chrono>
#include <cstring>
#include <curl/curl.h>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>
#include <udpsocket.h>
#include <stdlib.h>

#define GRACETIME 5

std::mutex logmtx;
std::string logstr;
std::atomic_bool run_service;
std::string lobby = "https://oldbox.orlandoviols.com/";

CURL* curl;

void update_config(std::string& deviceid, std::string& lobby)
{
  // test for config file on raspi:
  std::string config(get_file_contents("/boot/ov-client.cfg"));
  if(config.empty() && std::getenv("HOME"))
    // we are not on a raspi, or no file was created, thus check in home
    // directory
    config = get_file_contents(std::string(std::getenv("HOME")) +
                               std::string("/.ov-client.cfg"));
  if(config.empty())
    // we are not on a raspi, or no file was created, thus check in local
    // directory
    config = get_file_contents("ov-client.cfg");
  nlohmann::json js_cfg({{"deviceid", getmacaddr()},
                         {"url", "https://oldbox.orlandoviols.com/"}});
  if(!config.empty()) {
    try {
      js_cfg = nlohmann::json::parse(config);
    }
    catch(const std::exception& err) {
      DEBUG(config);
      DEBUG(err.what());
    }
  }
  deviceid = js_cfg.value("deviceid", deviceid);
  lobby = ovstrrep(js_cfg.value("url", lobby), "\\/", "/");
  if(deviceid.empty())
    deviceid = getmacaddr();
}

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

bool upload_log(const std::string& deviceid, const std::string& str, bool init)
{
  struct webCURL::MemoryStruct chunk;
  chunk.memory =
      (char*)malloc(1); /* will be grown as needed by the realloc above */
  chunk.size = 0;       /* no data at this point */
  std::string url =
      lobby + "?sendlog=" + deviceid + "&clear=" + std::to_string((int)init);
  curl_easy_reset(curl);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCURL::WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str.c_str());
  CURLcode err = CURLE_OK;
  if((err = curl_easy_perform(curl)) != CURLE_OK) {
    free(chunk.memory);
    return false;
  }
  free(chunk.memory);
  return true;
}

void service()
{
  curl = curl_easy_init();
  if(!curl) {
    std::cerr << "Unable to initialize curl.\n";
    return;
  }
  std::string deviceid;
  bool init = true;
  bool first = true;
  while(run_service || first) {
    // wait some seconds between uploads:
    TASCAR::tictoc_t tictoc;
    while(run_service && (tictoc.toc() < GRACETIME))
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if(deviceid.empty()) {
      // if devicename is still empty then try to get current MAC address:
      deviceid = getmacaddr();
      if(!deviceid.empty()) {
        std::lock_guard<std::mutex> lk(logmtx);
        std::cout << "Uploading log to " + lobby + " for device \"" + deviceid +
                         "\".\n";
      }
    }
    if(!deviceid.empty()) {
      // if device name is not empty, then upload next section of log string:
      std::lock_guard<std::mutex> lk(logmtx);
      if(upload_log(deviceid, logstr, init)) {
        init = false;
        logstr.clear();
      } else {
        std::cout << "Upload failed.\n";
      }
    }
    first = false;
  }
  {
    std::lock_guard<std::mutex> lk(logmtx);
    std::cout << "Ending upload service.\n";
  }
}

int main(int argc, char** argv)
{
  // parse command line parameters:
  const char* options = "s:h";
  struct option long_options[] = {
      {"server", 1, 0, 's'}, {"help", 0, 0, 'h'}, {0, 0, 0, 0}};
  int opt(0);
  int option_index(0);
  while((opt = getopt_long(argc, argv, options, long_options, &option_index)) !=
        -1) {
    switch(opt) {
    case 'h':
      app_usage("ovbox_sendlog", long_options, "");
      return 0;
    case 's':
      lobby = optarg;
      break;
    }
  }
  std::string sline;
  std::thread srv;
  // create a second thread for sending log to server:
  run_service = true;
  srv = std::thread(service);
  // read from stdin, copy to stdout and log string:
  while(!std::cin.eof() && std::cin.good()) {
    std::getline(std::cin, sline);
    if(std::cin.good()) {
      {
        std::lock_guard<std::mutex> lk(logmtx);
        std::cout << sline << std::endl;
        logstr += sline;
        logstr += '\n';
      }
    }
  }
  // end sender thread when input ends:
  run_service = false;
  if(srv.joinable())
    srv.join();
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
