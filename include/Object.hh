// Copyright 2018 Keri Oleg

#pragma once

#include "Model.hh"

namespace neat {

class Object : private NoCopy {
    const Model& model_;
    const glm::mat4& vp_;
    const glm::mat4& view_;
    glm::mat4 mm_;

  public:
    Object(const Model& model, const glm::mat4& vp, const glm::mat4& view,
        glm::mat4&& modelMatrix) noexcept;
    void render();
    void translate(const glm::vec3& t);
    void rotate(float angle, const glm::vec3& r);
};

}  // namespace neat
