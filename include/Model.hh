// Copyright 2018 Keri Oleg

#pragma once

#include <memory>
#include <string_view>
#include <glm/mat4x4.hpp>

#include "NoCopy.hh"

namespace neat {

class Model : private NoCopy {
    class Impl;
    std::unique_ptr<Impl> pImpl_;

  public:
    Model(std::string_view filename, float scale) noexcept;
    Model(Model&& rhs) noexcept;
    void render(const glm::mat4& mvp) const;
    bool valid() const;
    ~Model();
};

}  // namespace neat
