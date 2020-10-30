#include "soundcardtools.h"
#ifndef __APPLE__
#include <alsa/asoundlib.h>
#endif


#define MA_LOG_LEVELVERBOSE
#define MA_NO_JACK
#define MA_NO_WEBAUDIO
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"


void mini_device_print()
{
  ma_result result;
  ma_context context;
  ma_device_info* pPlaybackDeviceInfos;
  ma_uint32 playbackDeviceCount;
  ma_device_info* pCaptureDeviceInfos;
  ma_uint32 captureDeviceCount;
  ma_uint32 iDevice;

  if(ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
    printf("Failed to initialize context.\n");
    // return -2;
  }

  result = ma_context_get_devices(&context, &pPlaybackDeviceInfos,
                                  &playbackDeviceCount, &pCaptureDeviceInfos,
                                  &captureDeviceCount);
  if(result != MA_SUCCESS) {
    printf("Failed to retrieve device information.\n");
    // return -3;
  }

  printf("\n\nPlayback Devices \n\n");
  for(iDevice = 0; iDevice < playbackDeviceCount; ++iDevice) {


    if (ma_context_get_device_info(&context,
                                     ma_device_type_playback,
                                    &pPlaybackDeviceInfos[iDevice].id,
                                    ma_share_mode_shared,
                                     &pPlaybackDeviceInfos[iDevice])
            != MA_SUCCESS)
            continue;
        printf("+---------------- %u: %s\n", iDevice, pPlaybackDeviceInfos[iDevice].name);
        //printf("|                 id: %u\n", pPlaybackDeviceInfos[iDevice].id);
        printf("|max output channels: %u\n", pPlaybackDeviceInfos[iDevice].maxChannels);
        printf("|     min samplerate: %u\n", pPlaybackDeviceInfos[iDevice].minSampleRate);
        printf("|     max samplerate: %u\n", pPlaybackDeviceInfos[iDevice].maxSampleRate);
        printf("+----------------------------------------+\n\n\n");

  }

  printf("\n\n\n");

  printf("\n\nCapture Devices \n\n");
  for(iDevice = 0; iDevice < captureDeviceCount; ++iDevice) {

    if (ma_context_get_device_info(&context, ma_device_type_capture,
                                       &pCaptureDeviceInfos[iDevice].id, ma_share_mode_shared, &pCaptureDeviceInfos[iDevice])
            != MA_SUCCESS)
            continue;
        printf("+----------------%u: %s\n", iDevice, pCaptureDeviceInfos[iDevice].name);
        //printf("|                id: %u\n", pCaptureDeviceInfos[iDevice].id);
        printf("|max input channels: %u\n", pCaptureDeviceInfos[iDevice].maxChannels);
        printf("|    min samplerate: %u\n", pCaptureDeviceInfos[iDevice].minSampleRate);
        printf("|    max samplerate: %u\n", pCaptureDeviceInfos[iDevice].maxSampleRate);
        printf("+-----------------------------------------+\n\n\n");
  }

  ma_context_uninit(&context);
}



std::vector<snddevname_t> list_sound_devices_miniaudio()
{
  std::vector<snddevname_t> retv;
}

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
#endif

#ifdef __APPLE__

#endif // __APPLE__
  return retv;
}



/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
