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

#include "Model.hh"

namespace neat {

class Object {
    const Model& model_;
    const glm::mat4& vp_;
    const glm::mat4& view_;
    glm::mat4 mm_;

  public:
    Object(const Model& model, const glm::mat4& vp, const glm::mat4& view,
        const glm::mat4& modelMatrix = glm::mat4(1.f)) noexcept;
    void render() const;
    void translate(const glm::vec3& t);
    void setPosition(const glm::mat4& p);
    [[nodiscard]] glm::mat4 position() const;
    void rotate(float angle, const glm::vec3& r);
};

}  // namespace neat
