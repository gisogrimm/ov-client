#include "ovboxclient.h"
#include <condition_variable>
#include <map>
#include <netdb.h>
#include <strings.h>
#include <thread>

ovboxclient_t::ovboxclient_t(const std::string& desthost, port_t destport,
                             port_t recport, port_t portoffset, int prio,
                             secret_t secret, stage_device_id_t callerid,
                             bool peer2peer_, bool donotsend_,
                             bool downmixonly_)
    : prio(prio), secret(secret), remote_server(secret), toport(destport),
      recport(recport), portoffset(portoffset), callerid(callerid),
      runsession(true), mode(0)
{
  if(peer2peer_)
    mode |= B_PEER2PEER;
  if(downmixonly_)
    mode |= B_DOWNMIXONLY;
  if(donotsend_)
    mode |= B_DONOTSEND;
  local_server.set_timeout_usec(100000);
  local_server.destination("localhost");
  local_server.bind(recport, true);
  remote_server.destination(desthost.c_str());
  remote_server.bind(0, false);
  localep = getipaddr();
  localep.sin_port = remote_server.getsockep().sin_port;
  sendthread = std::thread(&ovboxclient_t::sendsrv, this);
  recthread = std::thread(&ovboxclient_t::recsrv, this);
  pingthread = std::thread(&ovboxclient_t::pingservice, this);
}

ovboxclient_t::~ovboxclient_t()
{
  runsession = false;
  DEBUG(1);
  sendthread.join();
  DEBUG(1);
  recthread.join();
  DEBUG(1);
  pingthread.join();
  DEBUG(1);
}

void ovboxclient_t::add_extraport(port_t dest)
{
  xdest.push_back(dest);
}

void ovboxclient_t::announce_new_connection(stage_device_id_t cid,
                                            const ep_desc_t& ep)
{
  log(recport,
      "new connection for " + std::to_string(cid) + " from " + ep2str(ep.ep) +
          " in " + ((ep.mode & B_PEER2PEER) ? "peer-to-peer" : "server") +
          "-mode" + ((ep.mode & B_DOWNMIXONLY) ? " downmixonly" : "") +
          ((ep.mode & B_DONOTSEND) ? " donotsend" : "") + " v" + ep.version);
}

void ovboxclient_t::announce_connection_lost(stage_device_id_t cid)
{
  log(recport, "connection for " + std::to_string(cid) + " lost.");
}

void ovboxclient_t::announce_latency(stage_device_id_t cid, double lmin,
                                     double lmean, double lmax,
                                     uint32_t received, uint32_t lost)
{
  char ctmp[1024];
  if(lmean > 0) {
    sprintf(ctmp, "latency %d min=%1.2fms, mean=%1.2fms, max=%1.2fms", cid,
            lmin, lmean, lmax);
    log(recport, ctmp);
  }
  sprintf(ctmp, "packages from %d received=%d lost=%d (%1.2f%%)", cid, received,
          lost, 100.0 * (double)lost / (double)(std::max(1u, received + lost)));
  log(recport, ctmp);
  double data[6];
  data[0] = cid;
  data[1] = lmin;
  data[2] = lmean;
  data[3] = lmax;
  data[4] = received;
  data[5] = lost;
  char buffer[BUFSIZE];
  size_t n = packmsg(buffer, BUFSIZE, secret, callerid, PORT_PEERLATREP, 0,
                     (const char*)data, 6 * sizeof(double));
  remote_server.send(buffer, n, toport);
}

void ovboxclient_t::handle_endpoint_list_update(stage_device_id_t cid,
                                                const endpoint_t& ep)
{
  DEBUG(cid);
  DEBUG(ep2str(ep));
}

// ping service
void ovboxclient_t::pingservice()
{
  while(runsession) {
    std::this_thread::sleep_for(std::chrono::milliseconds(PINGPERIODMS));
    // send registration to server:
    remote_server.send_registration(callerid, mode, toport, localep);
    // send ping to other peers:
    size_t ocid(0);
    for(auto ep : endpoints) {
      if(ep.timeout && (ocid != callerid)) {
        remote_server.send_ping(callerid, ep.ep);
      }
      ++ocid;
    }
  }
}

// this thread receives messages from the server:
void ovboxclient_t::sendsrv()
{
  try {
    set_thread_prio(prio);
    char buffer[BUFSIZE];
    endpoint_t sender_endpoint;
    stage_device_id_t rcallerid;
    port_t destport;
    while(runsession) {
      size_t n(BUFSIZE);
      size_t un(BUFSIZE);
      sequence_t seq(0);
      char* msg(remote_server.recv_sec_msg(buffer, n, un, rcallerid, destport,
                                           seq, sender_endpoint));
      if(msg) {
        // the first port numbers are reserved for the control infos:
        if(destport > MAXSPECIALPORT) {
          if(rcallerid != callerid) {
            sequence_t dseq(seq - endpoints[rcallerid].seq);
            if(dseq != 0) {
              local_server.send(msg, un, destport + portoffset);
              for(auto xd : xdest)
                local_server.send(msg, un, destport + xd);
              ++endpoints[rcallerid].num_received;
              if(dseq < 0) {
                // report sequence error:
                size_t un =
                    packmsg(buffer, BUFSIZE, secret, callerid, PORT_SEQREP, 0,
                            (char*)(&rcallerid), sizeof(rcallerid));
                un = addmsg(buffer, BUFSIZE, un, (char*)(&dseq), sizeof(dseq));
                remote_server.send(buffer, un, toport);
              } else {
                endpoints[rcallerid].num_lost += (dseq - 1);
              }
              endpoints[rcallerid].seq = seq;
            }
          }
        } else {
          switch(destport) {
          case PORT_PING:
            msg_port(buffer) = PORT_PONG;
            msg_callerid(buffer) = callerid;
            remote_server.send(buffer, n, sender_endpoint);
            break;
          case PORT_PONG:
            if(rcallerid != callerid) {
              double tms(get_pingtime(msg, un));
              if(tms > 0)
                cid_setpingtime(rcallerid, tms);
            }
            break;
          case PORT_SETLOCALIP:
            if(un == sizeof(endpoint_t)) {
              cid_setlocalip(rcallerid, *((endpoint_t*)msg));
            }
            break;
          case PORT_LISTCID:
            if(un == sizeof(endpoint_t)) {
              // seq is peer2peer flag:
              cid_register(rcallerid, *((endpoint_t*)msg), seq, "");
            }
            break;
          }
        }
      }
    }
  }
  catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    runsession = false;
  }
}

// this thread receives local UDP messages and handles them:
void ovboxclient_t::recsrv()
{
  try {
    set_thread_prio(prio);
    char buffer[BUFSIZE];
    char msg[BUFSIZE];
    endpoint_t sender_endpoint;
    log(recport, "listening");
    sequence_t seq(0);
    while(runsession) {
      ssize_t n = local_server.recvfrom(buffer, BUFSIZE, sender_endpoint);
      if(n > 0) {
        ++seq;
        size_t un =
            packmsg(msg, BUFSIZE, secret, callerid, recport, seq, buffer, n);
        bool sendtoserver(!(mode & B_PEER2PEER));
        if(mode & B_PEER2PEER) {
          size_t ocid(0);
          for(auto ep : endpoints) {
            if(ep.timeout) {
              if((ocid != callerid) && (ep.mode & B_PEER2PEER) &&
                 (!(ep.mode & B_DONOTSEND))) {
                remote_server.send(msg, un, ep.ep);
              } else {
                sendtoserver = true;
              }
            }
            ++ocid;
          }
        }
        if(sendtoserver) {
          remote_server.send(msg, un, toport);
        }
      }
    }
  }
  catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    runsession = false;
  }
  DEBUG(1);
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
