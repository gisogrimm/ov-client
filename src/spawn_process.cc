#include "spawn_process.h"
#include "errmsg.h"
#include "string.h"
#include <signal.h>

spawn_process_t::spawn_process_t(const std::string& command)
    : h_pipe(NULL), pid(0)
{
  if(!command.empty()) {
    size_t len(command.size() + 100);
    char ctmp[len];
    memset(ctmp, 0, len);
    snprintf(ctmp, len, "sh -c \"%s >/dev/null & echo \\$!\"", command.c_str());
    h_pipe = popen(ctmp, "r");
    if(!h_pipe)
      throw ErrMsg("Unable to start pipe with command \"" + std::string(ctmp) +
                   "\".");
    if(fgets(ctmp, 1024, h_pipe) != NULL) {
      pid = atoi(ctmp);
      if(pid == 0)
        throw ErrMsg("Invalid subprocess PID (0) when starting command \"" +
                     command + "\".");
    }
  }
}

spawn_process_t::~spawn_process_t()
{
  if(pid != 0)
    kill(pid, SIGTERM);
  if(h_pipe)
    fclose(h_pipe);
}

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
