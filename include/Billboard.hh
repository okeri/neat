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
    explicit Billboard(glm::vec4* vertices) noexcept;
    Billboard(Billboard&& rhs) noexcept;
    void render(const Texture& texture) const noexcept;
};

#define defineStdRect(l, t, r, b)                                       \
    {                                                                   \
        {l, t, 1., 0.}, {l, b, 1., 1.}, {r, b, 0., 1.}, {r, b, 0., 1.}, \
            {r, t, 0., 0.}, {                                           \
            l, t, 1., 0                                                 \
        }                                                               \
    }
}  // namespace neat
