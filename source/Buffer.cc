// Copyright 2018 Keri Oleg

#include <GLES3/gl3.h>

#include <utility>

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
