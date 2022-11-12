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

#include <glm/gtc/type_ptr.hpp>

#include <Object.hh>

namespace neat {

Object::Object(const Model& model, const glm::mat4& vp, const glm::mat4& view,
    const glm::mat4& modelMatrix) noexcept :
    model_(model), vp_(vp), view_(view), modelMatrix_(modelMatrix) {
}

void Object::render() const {
    model_.render(vp_ * modelMatrix_, view_ * modelMatrix_,
        glm::transpose(glm::inverse(modelMatrix_)));
}

void Object::translate(const glm::vec3& t) {
    modelMatrix_ = glm::translate(modelMatrix_, t);
}

void Object::rotate(float angle, const glm::vec3& r) {
    modelMatrix_ = glm::rotate(modelMatrix_, angle, r);
}

void Object::setPosition(const glm::mat4& p) {
    modelMatrix_ = p;
}

glm::mat4 Object::position() const {
    return modelMatrix_;
}

}  // namespace neat
