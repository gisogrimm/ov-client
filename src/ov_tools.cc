#include "ov_tools.h"
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef DEBUG
#define DEBUG(x)                                                               \
  std::cerr << __FILE__ << ":" << __LINE__ << ": " << #x << "=" << x           \
            << std::endl
#endif

#ifdef LINUX
static void get_process_name(const pid_t pid, char* name, char* arg)
{
  name[0] = '\0';
  arg[0] = '\0';
  char procfile[BUFSIZ];
  sprintf(procfile, "/proc/%d/cmdline", pid);
  FILE* f = fopen(procfile, "r");
  if(f) {
    size_t size;
    size = fread(name, sizeof(char), sizeof(procfile), f);
    if(size > 0) {
      if('\n' == name[size - 1])
        name[size - 1] = '\0';
      if(size > strlen(name)) {
        strcpy(arg, &(name[strlen(name) + 1]));
      }
    }
    fclose(f);
  }
}
#endif

bool is_ovbox()
{
#ifdef LINUX
  // check if parent is the autorun script:
  pid_t ppid(getppid());
  char name[BUFSIZ];
  char arg[BUFSIZ];
  get_process_name(ppid, name, arg);
  DEBUG(ppid);
  DEBUG(name);
  DEBUG(arg);
  return strcmp(arg, "/home/pi/autorun") == 0;
#else
  // if not on LINUX this can not be an ovbox:
  return false;
#endif
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
