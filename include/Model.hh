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

#include <string_view>
#include <vector>

#include <glm/mat4x4.hpp>

#include "NoCopy.hh"
#include "PImpl.hh"

namespace neat {

class Model : private NoCopy {
    class Impl;

    PImpl<Impl, 128, 8> pImpl_;

  public:
    explicit Model(std::string_view filename) noexcept;
    Model(Model&& rhs) noexcept;
    ~Model() noexcept;

    void setPos(const glm::mat4& pos) const noexcept;
    void setPos(const std::vector<glm::mat4>& pos) const noexcept;
    void render(unsigned instances = 1) const noexcept;

    [[nodiscard]] bool valid() const noexcept;

    static void setLight(unsigned index, const glm::vec3& position,
        const glm::vec3& color, float attenuation) noexcept;
    static void setSun(
        const glm::vec3& direction, const glm::vec3& color) noexcept;
    static void setLightningShader(bool fragment) noexcept;
    static void setVP(const glm::mat4& v, const glm::mat4& p) noexcept;
};

}  // namespace neat
