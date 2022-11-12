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

#pragma once

#include <string_view>
#include <vector>

#ifdef ANDROID
#include <android/asset_manager.h>
#else
#include <fstream>
#endif

namespace neat {

class Asset {
#ifdef ANDROID
    AAsset* file_;
#else
    std::ifstream file_;
#endif

  public:
    enum { Error = 0xffffffffffffffff };

    explicit Asset(std::string_view filename) noexcept;
    ~Asset();
    bool valid();
    bool open(std::string_view filename);
    void close();
    uint64_t read(void* data, uint64_t size);
    uint64_t read(std::vector<uint8_t>& data);
    uint64_t seek(uint64_t ofs);

#ifdef ANDROID
    static void setManager(AAssetManager* manager);
#endif
};

}  // namespace neat
