#ifndef ERRMSG_H
#define ERRMSG_H

#include <string>

class ErrMsg : public std::exception, private std::string {
public:
  ErrMsg(const std::string& msg);
  ErrMsg(const std::string& msg, int err);
  virtual ~ErrMsg() throw();
  const char* what() const throw();
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
