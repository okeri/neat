// Copyright 2018 Keri Oleg

#pragma once

#include <vector>

#include "GLResource.hh"

namespace neat {

class Buffer : private GLResource {
  public:
    enum class Target { Array = 0x8892, ElementArray = 0x8893 };
    Buffer() noexcept;
    Buffer(Buffer&& rhs) noexcept;
    ~Buffer();
    Buffer& operator=(Buffer&& rhs) noexcept;
    void bind(Target target = Target::Array) const;
    void unbind(Target target = Target::Array) const;
    void set(const void* data, unsigned size);

    template <typename _Tp>
    void set(const std::vector<_Tp>& data) {
        set(data.data(), data.size() * sizeof(_Tp));
    }
};

}  // namespace neat
