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

#include <vector>

#include "GLResource.hh"

namespace neat {

class Buffer : private GLResource {
  public:
    enum class Target : unsigned { Array = 0x8892, ElementArray = 0x8893 };
    explicit Buffer(
        Target target = Target::Array, bool dynamic = false) noexcept;
    Buffer(Buffer&& rhs) noexcept;
    ~Buffer();
    Buffer& operator=(Buffer&& rhs) noexcept;
    void bind() const noexcept;
    void unbind() const noexcept;
    void set(const void* data, std::size_t size) const noexcept;
    [[nodiscard]] unsigned size() const noexcept;

    template <typename _Tp>
    void set(const std::vector<_Tp>& data) const noexcept {
        set(data.data(), data.size() * sizeof(_Tp));
    }

    static void unbind(Target target);

  private:
    Target target_;
    bool dynamic_;
};

}  // namespace neat
