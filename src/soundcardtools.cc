#include "soundcardtools.h"
#include <iostream>
#ifndef __APPLE__
#include <alsa/asoundlib.h>
#endif
#include "common.h"

std::vector<snddevname_t> list_sound_devices()
{
  std::vector<snddevname_t> retv;
#ifndef __APPLE__
  char** hints;
  int err;
  char** n;
  char* name;
  char* desc;

  /* Enumerate sound devices */
  err = snd_device_name_hint(-1, "pcm", (void***)&hints);
  if(err != 0) {
    std::cerr << "Warning: unable to get name hints (list_sound_devices)"
              << std::endl;
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
      if(dname.desc.find("\n") != std::string::npos)
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
  if(retv.empty()) {
    int card(-1);
    while(snd_card_next(&card) == 0) {
      if(card == -1)
        break;
      snddevname_t dname;
      char* card_name(NULL);
      if(0 == snd_card_get_name(card, &card_name)) {
        dname.dev = "hw:" + std::to_string(card);
        dname.desc = card_name;
        free(card_name);
        retv.push_back(dname);
      }
    }
  }
#endif
  return retv;
}

std::string url2localfilename(const std::string& url)
{
  if(url.empty())
    return url;
  std::string extension(url);
  size_t pos = extension.find("?");
  if(pos != std::string::npos)
    extension.erase(pos, extension.size() - pos);
  pos = extension.find(".");
  if(pos == std::string::npos)
    extension = "";
  else {
    while((pos = extension.find(".")) != std::string::npos)
      extension.erase(0, pos + 1);
    if(!extension.empty())
      extension = "." + extension;
  }
  return std::to_string(std::hash<std::string>{}(url)) + extension;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
