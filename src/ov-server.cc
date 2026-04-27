#include <arpa/inet.h>

#include "../tascar/libtascar/include/tscconfig.h"
#include "callerlist.h"
#include "common.h"
#include "errmsg.h"
#include "ovtcpsocket.h"
#include "udpsocket.h"
#include <condition_variable>
#include <queue>
#include <signal.h>
#include <string.h>
#include <thread>
#include <vector>

#include <curl/curl.h>

CURL* curl;
std::mutex curlmtx;

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

  CURLcode curl_get_http_response(const std::string& url,
                                  const std::string& userpwd, std::string& retv)
  {
    // std::string retv;
    struct webCURL::MemoryStruct chunk;
    chunk.memory =
        (char*)malloc(1); /* will be grown as needed by the realloc above */
    chunk.size = 0;       /* no data at this point */
    std::lock_guard<std::mutex> lock(curlmtx);
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    if(userpwd.size())
      curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCURL::WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    // curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    // curl_easy_setopt(curl, CURLOPT_POSTFIELDS, curlstrdevice.c_str());
    CURLcode res = curl_easy_perform(curl);
    if(res == CURLE_OK) {
      retv.clear();
      retv.insert(0, chunk.memory, chunk.size);
    } else {
      DEBUG(url);
      DEBUG(curl_easy_strerror(res));
    }
    free(chunk.memory);
    return res;
  }

} // namespace webCURL

class latreport_t {
public:
  latreport_t() : src(0), dest(0), tmean(0), jitter(0){};
  latreport_t(stage_device_id_t src_, stage_device_id_t dest_, double tmean_,
              double jitter_)
      : src(src_), dest(dest_), tmean(tmean_), jitter(jitter_){};
  stage_device_id_t src;
  stage_device_id_t dest;
  double tmean;
  double jitter;
};

// period time of participant list announcement, in ping periods:
#define PARTICIPANTANNOUNCEPERIOD 20

// Ping period in Milliseconds:
#define PINGPERIODMS 50

// Announcement period time upon success, in ms:
#define ANNOUNCEMENTPERIOD_SUCCESS_MS 300000

#define ANNOUNCEMENTPERIOD_FAILURE_MS 50000

static bool quit_app(false);

class ov_server_t : public endpoint_list_t {
public:
  ov_server_t(int portno, int prio, const std::string& group_);
  ~ov_server_t();
  int portno;
  void srv();
  void announce_new_connection(stage_device_id_t cid, const ep_desc_t& ep);
  void announce_connection_lost(stage_device_id_t cid);
  void announce_latency(stage_device_id_t cid, double lmin, double lmean,
                        double lmax, uint32_t received, uint32_t lost);
  void set_lobbyurl(const std::string& url)
  {
    std::lock_guard<std::mutex> lk(settings_mtx);
    lobbyurl = url;
  };
  void set_roomname(const std::string& name)
  {
    std::lock_guard<std::mutex> lk(settings_mtx);
    roomname = name;
  };
  void start_services();
  void stop_services();

private:
  void jittermeasurement_service();
  std::thread jittermeasurement_thread;
  void announce_service();
  std::thread announce_thread;
  void ping_and_callerlist_service();
  std::thread logthread;
  void quitwatch();
  std::thread quitthread;
  const int prio = 0;

  secret_t secret = 1234;
  ovbox_udpsocket_t socket;
  std::atomic<bool> runsession{false};
  std::string roomname = "";
  std::string lobbyurl = "http://localhost";
  std::mutex settings_mtx;

  std::queue<latreport_t> latfifo;
  std::mutex latfifomtx;

  double serverjitter = -1.0;

  std::string group;
};

ov_server_t::ov_server_t(int portno_, int prio, const std::string& group_)
    : portno(portno_), prio(prio), socket(secret, STAGE_ID_SERVER),
      roomname(addr2str(getipaddr().sin_addr) + ":" + std::to_string(portno)),
      group(group_)
{
  endpoints.resize(255, ep_desc_t());
  // for(auto& ep:endpoints)
  //  memset(&ep,0,sizeof(ep));
  socket.set_timeout_usec(100000);
  portno = socket.bind(portno);
}

ov_server_t::~ov_server_t()
{
  if(runsession)
    stop_services();
  socket.close();
}

void ov_server_t::start_services()
{
  if(runsession)
    stop_services();
  {
    std::lock_guard<std::mutex> lk(settings_mtx);
    runsession = true;
    logthread = std::thread(&ov_server_t::ping_and_callerlist_service, this);
    quitthread = std::thread(&ov_server_t::quitwatch, this);
    announce_thread = std::thread(&ov_server_t::announce_service, this);
    jittermeasurement_thread =
        std::thread(&ov_server_t::jittermeasurement_service, this);
  }
}

void ov_server_t::stop_services()
{
  runsession = false;
  if(jittermeasurement_thread.joinable())
    jittermeasurement_thread.join();
  if(logthread.joinable())
    logthread.join();
  if(quitthread.joinable())
    quitthread.join();
  if(announce_thread.joinable())
    announce_thread.join();
}

void ov_server_t::quitwatch()
{
  while(!quit_app)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  runsession = false;
}

void ov_server_t::announce_new_connection(stage_device_id_t cid,
                                          const ep_desc_t& ep)
{
  log(portno,
      "new connection for " + std::to_string(cid) + " from " + ep2str(ep.ep) +
          " in " + ((ep.mode & B_PEER2PEER) ? "peer-to-peer" : "server") +
          "-mode" + ((ep.mode & B_RECEIVEDOWNMIX) ? " receivedownmix" : "") +
          ((ep.mode & B_SENDDOWNMIX) ? " senddownmix" : "") +
          ((ep.mode & B_DONOTSEND) ? " donotsend" : "") + " v" + ep.version);
}

void ov_server_t::announce_connection_lost(stage_device_id_t cid)
{
  log(portno, "connection for " + std::to_string(cid) + " lost.");
}

void ov_server_t::announce_latency(stage_device_id_t cid, double lmin,
                                   double lmean, double lmax, uint32_t received,
                                   uint32_t lost)
{
  if(lmean > 0) {
    {
      std::lock_guard<std::mutex> lk(latfifomtx);
      latfifo.push(latreport_t(cid, 200, lmean, lmax - lmean));
    }
    char ctmp[1024];
    sprintf(ctmp, "latency %d min=%1.2fms, mean=%1.2fms, max=%1.2fms", cid,
            lmin, lmean, lmax);
    log(portno, ctmp);
  }
}

// This function announces the room service to the lobby:
void ov_server_t::announce_service()
{
  try {
    // Room announcement counter:
    uint32_t announcementCounter(0);
    // The URL part containing the HTTP GET request:
    char httpGetRequest[1024];
    while(runsession) {
      if(!announcementCounter) {
        // Check if the room is empty:
        bool isRoomEmpty(false);
        // If nobody is connected, create a new pin:
        if(get_num_clients() == 0) {
          long int randomNumber(random());
          secret = randomNumber & 0xfffffff;
          socket.set_secret(secret);
          isRoomEmpty = true;
        }
        {
          std::lock_guard<std::mutex> lk(settings_mtx);
          // Register at the lobby:
          sprintf(httpGetRequest,
                  "?port=%d&name=%s&pin=%d&srvjit=%1.1f&grp=%s&version=%s",
                  portno, roomname.c_str(), secret, serverjitter, group.c_str(),
                  OVBOXVERSION);
          serverjitter = 0;
          std::string url(lobbyurl);
          url += std::string(httpGetRequest);
          if(isRoomEmpty) {
            // Tell the frontend that the room is not in use:
            url += "&empty=1";
          }
          std::string resp;
          CURLcode response =
              webCURL::curl_get_http_response(url, "room:room", resp);
          // If the request is successful, reconnect in 6000 periods (10
          // minutes), otherwise retry in 500 periods (50 seconds):
          if(response == CURLE_OK) {
            if(resp.size() == 0) {
              // expect empty response
              announcementCounter =
                  ANNOUNCEMENTPERIOD_SUCCESS_MS / PINGPERIODMS;
            } else {
              // non-empty response, probably invalid server or similar:
              std::cerr << "Error: invalid response from server:\n"
                        << resp << std::endl;
              announcementCounter =
                  ANNOUNCEMENTPERIOD_FAILURE_MS / PINGPERIODMS;
              std::cerr << "Request to " << url << " failed (invalid response)."
                        << std::endl;
            }
          } else {
            announcementCounter = ANNOUNCEMENTPERIOD_FAILURE_MS / PINGPERIODMS;
            std::cerr << "Request to " << url
                      << " failed: " << curl_easy_strerror(response)
                      << std::endl;
          }
        }
      }
      --announcementCounter;
      std::this_thread::sleep_for(std::chrono::milliseconds(PINGPERIODMS));
      while(!latfifo.empty()) {
        latreport_t latencyReport;
        {
          std::lock_guard<std::mutex> lockGuard(latfifomtx);
          latencyReport = latfifo.front();
          latfifo.pop();
        }
        // Provide a latency report:
        sprintf(httpGetRequest,
                "?latreport=%d&src=%d&dest=%d&lat=%1.1f&jit=%1.1f", portno,
                latencyReport.src, latencyReport.dest, latencyReport.tmean,
                latencyReport.jitter);
        std::string url(lobbyurl);
        url += httpGetRequest;
        {
          std::lock_guard<std::mutex> lk(curlmtx);
          curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
          curl_easy_setopt(curl, CURLOPT_USERPWD, "room:room");
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
          CURLcode response = curl_easy_perform(curl);
          if(response != CURLE_OK) {
            std::cerr << "Request to " << url
                      << " failed: " << curl_easy_strerror(response)
                      << std::endl;
          }
        }
      }
    }
  }
  catch(const std::exception& e) {
    DEBUG(e.what());
  }
}

// this thread sends ping and participant list messages
void ov_server_t::ping_and_callerlist_service()
{
  char buffer[BUFSIZE];
  // participand announcement counter:
  uint32_t participantannouncementcnt(PARTICIPANTANNOUNCEPERIOD);
  while(runsession) {
    std::this_thread::sleep_for(std::chrono::milliseconds(PINGPERIODMS));
    // send ping message to all connected endpoints:
    for(stage_device_id_t cid = 0; cid != MAX_STAGE_ID; ++cid) {
      if(endpoints[cid].timeout) {
        // endpoint is connected
        socket.send_ping(endpoints[cid].ep);
      }
    }
    if(!participantannouncementcnt) {
      // announcement of connected participants to all clients:
      participantannouncementcnt = PARTICIPANTANNOUNCEPERIOD;
      for(stage_device_id_t cid = 0; cid != MAX_STAGE_ID; ++cid) {
        if(endpoints[cid].timeout) {
          socket.send_pubkey(endpoints[cid].ep);
          for(stage_device_id_t epl = 0; epl != MAX_STAGE_ID; ++epl) {
            if(endpoints[epl].timeout) {
              // source (epl) and target (cid) endpoint are alive, send info of
              // epl to cid:
              {
                // update registry:
                size_t n = packmsg(buffer, BUFSIZE, secret, epl, PORT_LISTCID,
                                   endpoints[epl].mode,
                                   (const char*)(&(endpoints[epl].ep)),
                                   sizeof(endpoints[epl].ep));
                socket.send(buffer, n, endpoints[cid].ep);
              }
              {
                // update local IP address:
                size_t n =
                    packmsg(buffer, BUFSIZE, secret, epl, PORT_SETLOCALIP, 0,
                            (const char*)(&(endpoints[epl].localep)),
                            sizeof(endpoints[epl].localep));
                socket.send(buffer, n, endpoints[cid].ep);
              }
              {
                // update public key, if available:
                if(endpoints[epl].has_pubkey) {
                  // DEBUG(bin2base64((uint8_t*)(endpoints[epl].pubkey),crypto_box_PUBLICKEYBYTES));
                  size_t n = packmsg(buffer, BUFSIZE, secret, epl, PORT_PUBKEY,
                                     0, (const char*)(endpoints[epl].pubkey),
                                     crypto_box_PUBLICKEYBYTES);
                  socket.send(buffer, n, endpoints[cid].ep);
                }
              }
            }
          }
        }
      }
    }
    --participantannouncementcnt;
  }
}

void ov_server_t::srv()
{
  set_thread_prio(prio);
  char buffer[BUFSIZE];
  char cmsg[BUFSIZE];
  log(portno, "Multiplex service started (version " OVBOXVERSION ")");
  endpoint_t sender_endpoint;
  stage_device_id_t sender_id = 0;
  port_t destport;
  while(runsession) {
    // n is the packed message lenght:
    size_t n(BUFSIZE);
    // un is the unpacked message length:
    size_t un(BUFSIZE);
    sequence_t seq(0);
    char* msg(socket.recv_sec_msg(buffer, n, un, sender_id, destport, seq,
                                  sender_endpoint));
    if(msg && (sender_id < MAX_STAGE_ID)) {
      // regular destination port, forward data:
      if(destport > MAXSPECIALPORT) {
        auto& src = endpoints[sender_id];
        if(src.mode & B_ENCRYPTION) {
          auto newlen = decryptmsg(cmsg, buffer, n, socket.recipient_public,
                                   socket.recipient_secret);
          memcpy(buffer, cmsg, newlen);
          n = newlen;
        }
        for(stage_device_id_t target_id = 0; target_id != MAX_STAGE_ID;
            ++target_id) {
          auto& dest = endpoints[target_id];
          if((target_id != sender_id) && (dest.timeout > 0) &&
             (!(dest.mode & B_DONOTSEND)) &&
             ((!(dest.mode & B_PEER2PEER)) || (!(src.mode & B_PEER2PEER))) &&
             ((bool)(dest.mode & B_RECEIVEDOWNMIX) ==
              (bool)(src.mode & B_SENDDOWNMIX))) {
            char* send_msg = buffer;
            size_t send_len = n;
            // now check for encryption:
            if((src.mode & B_ENCRYPTION) && (dest.mode & B_ENCRYPTION) &&
               dest.has_pubkey) {
              send_len = encryptmsg(cmsg, BUFSIZE, buffer, n, dest.pubkey);
              send_msg = cmsg;
            }
            socket.send(send_msg, send_len, dest.ep);
          }
        }
      } else {
        // this is a control message:
        switch(destport) {
        case PORT_SEQREP:
          // sequence error report:
          if(un == sizeof(sequence_t) + sizeof(stage_device_id_t)) {
            stage_device_id_t sender_cid(*(sequence_t*)msg);
            sequence_t seq(*(sequence_t*)(&(msg[sizeof(stage_device_id_t)])));
            char ctmp[1024];
            sprintf(ctmp, "sequence error %d sender %d %d", sender_id,
                    sender_cid, seq);
            log(portno, ctmp);
          }
          break;
        case PORT_PEERLATREP:
          // peer-to-peer latency report:
          if(un == 6 * sizeof(double)) {
            double* data((double*)msg);
            {
              std::lock_guard<std::mutex> lk(latfifomtx);
              latfifo.push(
                  latreport_t(sender_id, data[0], data[2], data[3] - data[2]));
            }
            char ctmp[1024];
            sprintf(ctmp,
                    "peerlat %d-%g min=%1.2fms, mean=%1.2fms, max=%1.2fms",
                    sender_id, data[0], data[1], data[2], data[3]);
            log(portno, ctmp);
            sprintf(ctmp, "packages %d-%g received=%g lost=%g (%1.2f%%)",
                    sender_id, data[0], data[4], data[5],
                    100.0 * data[5] / (std::max(1.0, data[4] + data[5])));
            log(portno, ctmp);
          }
          break;
        case PORT_PING_SRV:
        case PORT_PONG_SRV:
          if(un >= sizeof(stage_device_id_t)) {
            stage_device_id_t* pdestid((stage_device_id_t*)msg);
            if(*pdestid < MAX_STAGE_ID)
              socket.send(buffer, n, endpoints[*pdestid].ep);
          }
          break;
        case PORT_PONG: {
          // ping response:
          double tms(socket.get_pingtime(msg, un));
          if(tms > 0)
            cid_setpingtime(sender_id, tms);
        } break;
        case PORT_SETLOCALIP:
          // receive local IP address of peer:
          if(un == sizeof(endpoint_t)) {
            // endpoint_t* localep((endpoint_t*)msg);
            cid_setlocalip(sender_id, msg);
          }
          break;
        case PORT_REGISTER: {
          // register new client:
          // in the register packet the sequence is used to transmit
          // peer2peer flag:
          std::string rver("---");
          if(un > 0) {
            msg[un - 1] = 0;
            rver = msg;
          }
          cid_register(sender_id, (char*)(&sender_endpoint), seq, rver);
        } break;
        case PORT_PUBKEY: {
          cid_set_pubkey(sender_id, msg, un);
        } break;
        }
      }
    }
  }
  log(portno, "Multiplex service stopped");
}

double get_pingtime(std::chrono::high_resolution_clock::time_point& t1)
{
  std::chrono::high_resolution_clock::time_point t2(
      std::chrono::high_resolution_clock::now());
  std::chrono::duration<double> time_span =
      std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
  t1 = t2;
  return (1000.0 * time_span.count());
}

void ov_server_t::jittermeasurement_service()
{
  set_thread_prio(prio - 1);
  std::chrono::high_resolution_clock::time_point t1;
  get_pingtime(t1);
  while(runsession) {
    std::this_thread::sleep_for(std::chrono::microseconds(2000));
    double t(get_pingtime(t1));
    t -= 2.0;
    serverjitter = std::max(t, serverjitter);
  }
}

static void sighandler(int sig)
{
  quit_app = true;
}

int main(int argc, char** argv)
{
  TASCAR::console_log_show(true);
  std::cout << "ov-server " << OVBOXVERSION << std::endl;
  std::chrono::high_resolution_clock::time_point start(
      std::chrono::high_resolution_clock::now());
  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);
  signal(SIGPIPE, SIG_IGN);
  try {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    {
      std::lock_guard<std::mutex> lk(curlmtx);
      curl = curl_easy_init();
      if(!curl)
        throw ErrMsg("Unable to initialize curl.");
    }
    int portno(0);
    int prio(55);
    std::string roomname;
    std::string lobby("http://oldbox.orlandoviols.com");
    std::string group;
    bool usetcp = false;
    const char* options = "p:qr:hvn:l:g:";
    struct option long_options[] = {{"rtprio", 1, 0, 'r'},
                                    {"quiet", 0, 0, 'q'},
                                    {"port", 1, 0, 'p'},
                                    {"verbose", 0, 0, 'v'},
                                    {"help", 0, 0, 'h'},
                                    {"name", 1, 0, 'n'},
                                    {"lobbyurl", 1, 0, 'l'},
                                    {"group", 1, 0, 'g'},
                                    {
                                        "tcp",
                                        0,
                                        0,
                                        't',
                                    },
                                    {0, 0, 0, 0}};
    int opt(0);
    int option_index(0);
    while((opt = getopt_long(argc, argv, options, long_options,
                             &option_index)) != -1) {
      switch(opt) {
      case 'h':
        app_usage("roomservice", long_options, "");
        return 0;
      case 'q':
        verbose = 0;
        break;
      case 'p':
        portno = atoi(optarg);
        break;
      case 'v':
        verbose++;
        break;
      case 'r':
        prio = atoi(optarg);
        break;
      case 'n':
        roomname = optarg;
        break;
      case 'g':
        group = optarg;
        break;
      case 'l':
        lobby = optarg;
        break;
      case 't':
        usetcp = true;
        break;
      }
    }
    std::chrono::high_resolution_clock::time_point end(
        std::chrono::high_resolution_clock::now());
    unsigned int seed(
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count());
    seed += portno;
    // initialize random generator:
    srandom(seed);
    {
      ov_server_t rec(portno, prio, group);
      if(!roomname.empty())
        rec.set_roomname(roomname);
      if(!lobby.empty())
        rec.set_lobbyurl(lobby);
      ovtcpsocket_t tcp;
      if(usetcp) {
        tcp.bind(portno);
      }
      rec.start_services();
      rec.srv();
      rec.stop_services();
    }
    {
      std::lock_guard<std::mutex> lk(curlmtx);
      curl_easy_cleanup(curl);
      curl_global_cleanup();
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
