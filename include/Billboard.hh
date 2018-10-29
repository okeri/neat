// Copyright 2018 Keri Oleg

#pragma once

#include <glm/vec4.hpp>
#include <optional>

#include "NoCopy.hh"
#include "Buffer.hh"
#include "Texture.hh"
#include "Program.hh"

namespace neat {

class Billboard : private NoCopy {
    glm::vec4* vertices_;
    Buffer buffer_;
    inline static std::optional<Program> program_;

  public:
    Billboard(Billboard&& rhs) noexcept;
    Billboard(glm::vec4* vertices) noexcept;
    void render(const Texture& texture);
};

#define defineStdRect(l, t, r, b)                                       \
    {                                                                   \
        {l, t, 1., 0.}, {l, b, 1., 1.}, {r, b, 0., 1.}, {r, b, 0., 1.}, \
            {r, t, 0., 0.}, {                                           \
            l, t, 1., 0                                                 \
        }                                                               \
    }
}  // namespace neat
