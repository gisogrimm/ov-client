#include "../libov/tascar/libtascar/include/tscconfig.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include <signal.h>
#include <thread>
#include <unistd.h>

pid_t system2(const std::string& command, bool noshell = true)
{
  pid_t pid = -1;
#ifndef _WIN32 // Windows has no fork.
  pid = fork();
  DEBUG(pid);
  if(pid < 0) {
    return pid;
  } else if(pid == 0) {
    /// Close all other descriptors for the safety sake.
    for(int i = 3; i < 4096; ++i)
      ::close(i);
    DEBUG(setsid());
    DEBUG(pid);
    int retv = 0;
    if(noshell) {
      DEBUG(pid);
      std::vector<std::string> pars = TASCAR::str2vecstr(command);
      char* vpars[pars.size() + 1];
      for(size_t k = 0; k < pars.size(); ++k) {
        vpars[k] = strdup(pars[k].c_str());
      }
      vpars[pars.size()] = NULL;
      DEBUG(pid);
      if(pars.size()) {
        retv = execvp(pars[0].c_str(), vpars);
      }
      DEBUG(pid);
      for(size_t k = 0; k < pars.size(); ++k) {
        DEBUG(vpars[k]);
        free(vpars[k]);
      }
      DEBUG(pid);
    } else {
      DEBUG(pid);
      retv = execl("/bin/sh", "sh", "-c", command.c_str(), NULL);
      DEBUG(pid);
    }
    DEBUG(retv);
    _exit(1);
  }
#endif
  return pid;
}

int main(int argc, char** argv)
{
  auto pid = system2("jackd -d dummy", true);
  // auto pid = system2("sleep 20",false);
  DEBUG(pid);
  sleep(4);
  DEBUG(kill(pid, 0));
  DEBUG(killpg(pid, 0));
  DEBUG(killpg(pid, SIGTERM));
  sleep(4);
  DEBUG(kill(pid, 0));
  DEBUG(killpg(pid, 0));
  DEBUG(killpg(pid, SIGKILL));
  DEBUG(kill(pid, SIGKILL));
  sleep(4);
  DEBUG(kill(pid, 0));
  DEBUG(killpg(pid, 0));
  waitpid(pid, NULL, 0);
  DEBUG(kill(pid, 0));
  sleep(4);
  DEBUG(kill(pid, 0));
  // siginfo_t sinf;
  // DEBUG(waitid(P_PGID,pid,&sinf,WEXITED));
  // DEBUG(sinf.si_pid);
  // DEBUG(strerror(errno));
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
