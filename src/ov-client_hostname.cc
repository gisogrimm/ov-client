#include <udpsocket.h>

int main(int argc, char** argv)
{
  std::string devname(getmacaddr());
  if(devname.size() > 6)
    devname.erase(0, 6);
  devname = "_" + devname;
  std::cout << devname << std::endl;
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
