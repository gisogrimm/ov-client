#include "soundcardtools.h"
#include <iostream>

int main(int argc, char** argv)
{
  std::vector<snddevname_t> alsadevs(list_sound_devices());
  for(auto d : alsadevs)
    std::cout << d.dev << "  (" << d.desc << ")" << std::endl;
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
