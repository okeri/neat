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

#include <utility>

#include <GLES3/gl3.h>

#include <Buffer.hh>

namespace neat {

Buffer::Buffer(Buffer&& rhs) noexcept {
    swap(std::move(rhs));
}

Buffer::Buffer() noexcept {
    glGenBuffers(1, &id_);
}

Buffer& Buffer::operator=(Buffer&& rhs) noexcept {
    swap(std::move(rhs));
    return *this;
}

Buffer::~Buffer() {
    if (id_ != 0) {
        glDeleteBuffers(1, &id_);
    }
}

void Buffer::bind(Target target) const {
    glBindBuffer(static_cast<GLenum>(target), id_);
}

void Buffer::unbind(Target target) const {
    glBindBuffer(static_cast<GLenum>(target), 0);
}

void Buffer::set(const void* data, unsigned size) {
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

}  // namespace neat
