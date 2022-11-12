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

#include <optional>

#include <glm/gtc/type_ptr.hpp>
#include <GLES3/gl3.h>

#include <Texture.hh>
#include <Program.hh>

namespace neat {

class Material : private NoCopy {
    glm::vec3 diffuse_{0.f, 0.f, 0.f};
    glm::vec3 ambient_{1.f, 1.f, 1.f};
    glm::vec3 specular_{0.f, 0.f, 0.f};

    std::optional<Texture> texture_;

  public:
    Material() = default;
    Material(Material&& rhs) noexcept :
        diffuse_(rhs.diffuse_),
        ambient_(rhs.ambient_),
        specular_(rhs.specular_) {
        if (rhs.texture_) {
            texture_ = std::move(*rhs.texture_);
        }
    }

    Material& operator=(Material&& rhs) noexcept {
        diffuse_ = rhs.diffuse_;
        ambient_ = rhs.ambient_;
        specular_ = rhs.specular_;
        if (rhs.texture_) {
            texture_ = std::move(*rhs.texture_);
        }
        return *this;
    }

    void setTexture(Texture&& texture) noexcept {
        texture_ = std::move(texture);
    }

    void setAmbient(const glm::vec3& color) noexcept {
        ambient_ = color;
    }

    void setDiffuse(const glm::vec3& color) noexcept {
        diffuse_ = color;
    }

    void setSpecular(const glm::vec3& color) noexcept {
        specular_ = color;
    }

    void bind(Program& program) const {
        if (texture_) {
            texture_->bind();
        }
        glUniform3fv(
            program.uniform("material.diffuse"), 1, glm::value_ptr(diffuse_));
        glUniform3fv(
            program.uniform("material.ambient"), 1, glm::value_ptr(ambient_));
        glUniform3fv(
            program.uniform("material.specular"), 1, glm::value_ptr(specular_));
    }

    static void unbind() {
        Texture::unbind();
    }
};

}  // namespace neat
