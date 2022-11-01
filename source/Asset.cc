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

#include <Asset.hh>

namespace neat {

#ifdef ANDROID
AAssetManager* __manager;

void Asset::setManager(AAssetManager* manager) {
    __manager = manager;
}

#endif

Asset::Asset(std::string_view filename) noexcept {
    open(filename);
}

Asset::~Asset() {
    close();
}

void Asset::close() {
#ifdef ANDROID
    if (file_ != nullptr) {
        AAsset_close(file_);
    }
#else
    file_.close();
#endif
}

bool Asset::open(std::string_view filename) {
#ifdef ANDROID
    file_ =
        AAssetManager_open(__manager, filename.data(), AASSET_MODE_STREAMING);
#else
    file_.open(std::string("assets/") + std::string(filename),
        std::ios_base::in | std::ios::binary);
#endif
    return valid();
}

bool Asset::valid() {
#ifdef ANDROID
    return file_ != nullptr;
#else
    return file_.good();
#endif
}

uint64_t Asset::read(void* data, uint64_t size) {
#ifdef ANDROID
    return AAsset_read(file_, static_cast<uint8_t*>(data), size);
#else
    if (!file_.read(static_cast<char*>(data), size)) {
        return Error;
    }
    return file_.gcount();
#endif
}

uint64_t Asset::read(std::vector<uint8_t>& data) {
    if (!valid()) {
        return Error;
    }
#ifdef ANDROID
    auto size = AAsset_getLength64(file_);
#else
    file_.seekg(0, std::ios_base::end);
    auto size = file_.tellg();
    file_.seekg(0, std::ios_base::beg);
#endif
    data.resize(size);
    return read(data.data(), size);
}

uint64_t Asset::seek(uint64_t ofs) {
#ifdef ANDROID
    return AAsset_seek64(file, ofs, SEEK_CUR);
#else
    file_.seekg(ofs, std::ios_base::cur);
    return file_.tellg();
#endif
}

}  // namespace neat
