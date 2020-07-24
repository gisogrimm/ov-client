#include "errmsg.h"
#include <errno.h>
#include <string.h>

ErrMsg::ErrMsg(const std::string& msg) : std::string(msg) {}

ErrMsg::ErrMsg(const std::string& msg, int err)
    : std::string(msg + std::string(strerror(err)))
{
}

ErrMsg::~ErrMsg() throw() {}

const char* ErrMsg::what() const throw()
{
  return c_str();
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
