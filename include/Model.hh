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

#include <array>
#include <vector>
#include <string_view>
#include <glm/mat4x4.hpp>

#include "NoCopy.hh"
#include "PImpl.hh"

namespace neat {

class LightManager : NoCopy {
    static const unsigned maxLightCount = 16;
    static LightManager& instance();

    struct UniformCache {
        unsigned int position;
        unsigned int color;
    };

  public:
    class Light {
        glm::vec3 position_;
        glm::vec3 color_;

        friend class LightManager;

      public:
        Light(const glm::vec3& pos, const glm::vec3& col);
    };

    static void push(const Light& light);
    static void pop_back();
    static void update(unsigned index, const Light& light);
    static void clear();

  private:
    std::vector<Light> lights_;
    std::array<UniformCache, maxLightCount> cache_;
};

class Model : private NoCopy {
    class Impl;
    PImpl<Impl, 48, 8> pImpl_;

  public:
    Model(std::string_view filename, float scale) noexcept;
    Model(Model&& rhs) noexcept;
    void render(
        const glm::mat4& mvp, const glm::mat4& mv, const glm::mat4& nm) const;
    bool valid() const;
    ~Model();
};

}  // namespace neat
