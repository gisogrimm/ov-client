/*
 * Copyright (c) 2020 Giso Grimm
 */
/*
 * ov-client is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * ov-client is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with ov-client. If not, see <http://www.gnu.org/licenses/>.
 */

#include "soundcardtools.h"
#include <iostream>

int main(int argc, char** argv)
{
  std::vector<snddevname_t> alsadevs(list_sound_devices());
  for(auto d : alsadevs)
    std::cout << d.dev << "  (" << d.desc << ")" << std::endl;
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
