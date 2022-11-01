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

#include <glm/vec4.hpp>

#include "NoCopy.hh"
#include "PImpl.hh"

namespace neat {

class Font : private NoCopy {
    class Impl;
    PImpl<Impl, 3440, 4> pImpl_;

  public:
    Font(const void* data, std::size_t size, int height) noexcept;
    Font(Font&& rhs) noexcept;
    [[nodiscard]] float width(std::string_view) const noexcept;
    [[nodiscard]] float height() const noexcept;
    [[nodiscard]] float centerX(std::string_view) const noexcept;
    [[nodiscard]] bool valid() const noexcept;
    void bind() const noexcept;
    void unbind() const noexcept;
    [[nodiscard]] std::vector<glm::vec4> calculate(
        std::string_view, float x, float y) const noexcept;
    ~Font();
};

}  // namespace neat
