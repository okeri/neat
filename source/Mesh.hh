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

#include <GLES3/gl3.h>

#include <Buffer.hh>

namespace neat {

class Mesh : public Buffer {
    unsigned count_;
    unsigned materialIndex_;

  public:
    using Face = uint32_t;

    Mesh(const Face* faces, unsigned count, unsigned matIndex) noexcept :
        Buffer(Buffer::Target::ElementArray),
        count_(count),
        materialIndex_(matIndex) {
        Buffer::bind();
        Buffer::set(faces, count_ * sizeof(Face));
    }

    Mesh(Mesh&& rhs) noexcept :
        Buffer(std::move(rhs)),
        count_(rhs.count_),
        materialIndex_(rhs.materialIndex_) {
    }

    [[nodiscard]] unsigned materialIndex() const noexcept {
        return materialIndex_;
    }

    void render() const noexcept {
        glDrawElements(GL_TRIANGLES, count_, GL_UNSIGNED_INT, nullptr);
    }
};

}  // namespace neat
