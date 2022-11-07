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

Buffer::Buffer(Buffer&& rhs) noexcept : target_(rhs.target_) {
    swap(std::move(rhs));
}

Buffer::Buffer(Target target) noexcept : target_(target) {
    glGenBuffers(1, &id_);
}

Buffer& Buffer::operator=(Buffer&& rhs) noexcept {
    target_ = rhs.target_;
    swap(std::move(rhs));
    return *this;
}

Buffer::~Buffer() {
    if (id_ != 0) {
        glDeleteBuffers(1, &id_);
    }
}

void Buffer::bind() const noexcept {
    glBindBuffer(static_cast<GLenum>(target_), id_);
}

void Buffer::unbind() const noexcept {
    glBindBuffer(static_cast<GLenum>(target_), 0);
}

void Buffer::unbind(Target target) {
    glBindBuffer(static_cast<GLenum>(target), 0);
}

void Buffer::set(const void* data, unsigned size) const noexcept {
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

unsigned Buffer::size() const noexcept {
    GLint ret;
    glGetBufferParameteriv(static_cast<GLenum>(target_), GL_BUFFER_SIZE, &ret);
    return ret;
}

}  // namespace neat
