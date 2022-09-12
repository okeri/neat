// Copyright 2018 Keri Oleg

#pragma once

#include <string_view>
#include <vector>

#include <glm/vec2.hpp>
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
    float width(std::string_view) const;
    float height() const;
    float centerX(std::string_view) const;
    bool valid() const;
    void bind() const;
    void unbind() const;
    std::vector<glm::vec4> calculate(
        std::string_view, const glm::vec2& coords) const;
    ~Font();
};

}  // namespace neat
