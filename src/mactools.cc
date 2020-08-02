#include "mactools.h"

#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/ioctl.h>

std::string getmacaddr()
{
  std::string retv;
  struct ifreq ifr;
  struct ifconf ifc;
  char buf[1024];
  int success = 0;

  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if(sock == -1) { /* handle error*/
    return retv;
  };

  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  if(ioctl(sock, SIOCGIFCONF, &ifc) == -1) { /* handle error */
    return retv;
  }

  struct ifreq* it = ifc.ifc_req;
  const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

  for(; it != end; ++it) {
    strcpy(ifr.ifr_name, it->ifr_name);
    if(ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
      if(!(ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
        if(ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
          success = 1;
          break;
        }
      }
    } else { /* handle error */
      return retv;
    }
  }

  unsigned char mac_address[6];

  if(success) {
    memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
    char ctmp[1024];
    sprintf(ctmp, "%02x%02x%02x%02x%02x%02x", mac_address[0], mac_address[1],
            mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
    retv = ctmp;
  }
  return retv;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
