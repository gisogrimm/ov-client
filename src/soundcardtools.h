#ifndef SOUNDCARDTOOLS_H
#define SOUNDCARDTOOLS_H

#include <string>
#include <vector>

struct snddevname_t {
  std::string dev;
  std::string desc;
};

// get a list of available device names. Implementation may depend on OS and
// audio backend
std::vector<snddevname_t> list_sound_devices();

std::string url2localfilename(const std::string& url);

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
