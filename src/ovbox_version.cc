/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2020-2024 Giso Grimm
 */
/*
 * ovbox_version is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * ovbox_version is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with ovbox_version. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "ov_client_orlandoviols.h"
#include "ov_render_tascar.h"
#include <fstream>
#include <sys/utsname.h>

int main(int argc, char** argv)
{
  struct utsname buffer;
  errno = 0;
  if(uname(&buffer) != 0) {
    perror("uname");
    exit(EXIT_FAILURE);
  }
  std::cout << get_libov_version() << " " << buffer.sysname << " "
            << buffer.release << " (" << buffer.machine << ")"
            << "\n";
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
