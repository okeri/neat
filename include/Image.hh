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

#include <cstdint>

namespace neat {

class Image {
    uint8_t* data_;
    std::size_t size_;
    uint32_t width_;
    uint32_t height_;
    void load(const void* data, std::size_t size) noexcept;

  public:
    Image(const void* data, std::size_t size) noexcept;
    Image(Image&& other) noexcept;
    [[nodiscard]] uint8_t* data() const noexcept;
    [[nodiscard]] uint32_t width() const noexcept;
    [[nodiscard]] uint32_t height() const noexcept;
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] unsigned bpp() const noexcept;
    ~Image();
};

}  // namespace neat
