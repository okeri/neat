// Copyright 2018 Keri Oleg

#include <utility>

#include <GLResource.hh>

namespace neat {

GLResource::GLResource() : id_(0) {
}

void GLResource::swap(GLResource&& other) noexcept {
    std::swap(id_, other.id_);
}

}  // namespace neat
