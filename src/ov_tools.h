#ifndef OV_TOOLS_H
#define OV_TOOLS_H

#include <nlohmann/json.hpp>
#include <string>

// Return true if this process is called from ovbox autorun scripts
bool is_ovbox();

// download a file from an url and save as a given name
// bool download_file(const std::string& url, const std::string& dest);

// simple string replace function:
std::string ovstrrep(std::string s, const std::string& pat,
                     const std::string& rep);

// robust json value function with default value:
template <class T>
T my_js_value(nlohmann::json& obj, const std::string& key, const T& defval)
{
  if(obj.is_object())
    return obj.value(key, defval);
  return defval;
}

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
