/*
    neat - simple graphics engine
    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifdef ANDROID
#include <android/log.h>
#endif

#ifdef LINUX
#include <iostream>
#endif

#include <Log.hh>

namespace neat {

Log::~Log() {
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_INFO, tag, "%s", message_.str().c_str());
#endif

#ifdef LINUX
    std::cout << message_.str() << std::endl;
#endif
}

}  // namespace neat
