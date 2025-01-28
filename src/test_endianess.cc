#include "udpsocket.h"
#include <chrono>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
#ifdef WIN32
  WSADATA WSAData; // Structure to store details of the Windows Sockets
  if(WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
    return 1;
#endif
  std::cout << "test" << std::endl;
  fprintf(stderr,"ep:\n");
  endpoint_t ep = ovgethostbyname("localhost");
  uint8_t msg[sizeof(ep)];
  memcpy(msg, &ep, sizeof(ep));
  printf("sizeof(ep): %lu\n", sizeof(ep));
  printf("sizeof(char): %lu\n", sizeof(char));
  for(size_t k = 0; k < sizeof(ep); ++k)
    printf("%02x ", msg[k]);
  printf("\n\n");

  printf("tp:\n");
  std::chrono::high_resolution_clock::time_point tp =
      std::chrono::high_resolution_clock::now();
  tp = std::chrono::floor<std::chrono::hours>(tp);
  uint8_t msgtp[sizeof(tp)];
  memcpy(msgtp, &tp, sizeof(tp));
  printf("sizeof(std::chrono::high_resolution_clock::time_point): %lu\n",
         sizeof(std::chrono::high_resolution_clock::time_point));
  for(size_t k = 0; k < sizeof(tp); ++k)
    printf("%02x ", msgtp[k]);
  printf("\n");

#ifdef WIN32
  WSACleanup(); // Clean up Winsock
#endif
  return 0;
}
