// Copyright 2018 Keri Oleg

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
    glm::mat4 position() const;
    void rotate(float angle, const glm::vec3& r);
};

}  // namespace neat
