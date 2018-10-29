// Copyright 2018 Keri Oleg

#pragma once

#include <string_view>
#include <memory>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace neat {

class Font : private NoCopy {
    class Impl;
    std::unique_ptr<Impl> pImpl_;

  public:
    Font(const void* data, std::size_t size, int height) noexcept;
    Font(Font&& other) noexcept;
    float centerX(std::string_view) const;
    float height() const;
    bool valid() const;
    void bind() const;
    void unbind() const;
    std::vector<glm::vec4> calculate(
        std::string_view, const glm::vec2& coords) const;
    ~Font();
};

}  // namespace neat
