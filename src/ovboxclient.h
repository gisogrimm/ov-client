#ifndef OVBOXCLIENT
#define OVBOXCLIENT

#include "callerlist.h"
#include "common.h"
#include "udpsocket.h"

class ovboxclient_t : public endpoint_list_t {
public:
  ovboxclient_t(const std::string& desthost, port_t destport, port_t recport,
                port_t portoffset, int prio, secret_t secret,
                callerid_t callerid, bool peer2peer, bool donotsend,
                bool downmixonly);
  virtual ~ovboxclient_t();
  void announce_new_connection(callerid_t cid, const ep_desc_t& ep);
  void announce_connection_lost(callerid_t cid);
  void announce_latency(callerid_t cid, double lmin, double lmean, double lmax,
                        uint32_t received, uint32_t lost);
  void add_destination(port_t dest);

private:
  void sendsrv();
  void recsrv();
  void pingservice();
  void handle_endpoint_list_update(callerid_t cid, const endpoint_t& ep);
  // real time priority:
  const int prio;
  // PIN code to connect to server:
  secret_t secret;
  // data relay server address:
  ovbox_udpsocket_t remote_server;
  // local UDP receiver:
  udpsocket_t local_server;
  // additional port offsets to send data to locally:
  std::vector<port_t> xdest;
  port_t toport;
  port_t recport;
  // port offset for primary port, added to nominal port, e.g., in case of local
  // setup:
  port_t portoffset;
  // client/caller identification (aka 'chair' in the lobby system):
  callerid_t callerid;
  bool runsession;
  std::thread sendthread;
  std::thread recthread;
  std::thread pingthread;
  epmode_t mode;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
