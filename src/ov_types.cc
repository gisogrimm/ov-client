#include "ov_types.h"

bool operator!=(const TASCAR::pos_t& a, const TASCAR::pos_t& b)
{
  return (a.x != b.x) || (a.y != b.y) || (a.z != b.z);
}

bool operator!=(const TASCAR::zyx_euler_t& a, const TASCAR::zyx_euler_t& b)
{
  return (a.x != b.x) || (a.y != b.y) || (a.z != b.z);
}

bool operator!=(const audio_device_t& a, const audio_device_t& b)
{
  return (a.drivername != b.drivername) || (a.devicename != b.devicename) ||
         (a.srate != b.srate) || (a.periodsize != b.periodsize) ||
         (a.numperiods != b.numperiods);
}

bool operator!=(const device_channel_t& a, const device_channel_t& b)
{
  return (a.sourceport != b.sourceport) || (a.gain != b.gain) ||
         (a.position != b.position);
}

bool operator!=(const std::vector<device_channel_t>& a,
                const std::vector<device_channel_t>& b)
{
  if(a.size() != b.size())
    return true;
  for(size_t k = 0; k < a.size(); ++k)
    if(a[k] != b[k])
      return true;
  return false;
}

bool operator!=(const stage_device_t& a, const stage_device_t& b)
{
  return (a.id != b.id) || (a.label != b.label) || (a.channels != b.channels) ||
         (a.position != b.position) || (a.orientation != b.orientation) ||
         (a.gain != b.gain) || (a.mute != b.mute);
}
