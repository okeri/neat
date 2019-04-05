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
    inline glm::mat4& modelMatrix() {
        return mm_;
    }

    Object(const Model& model, const glm::mat4& vp, const glm::mat4& view,
        glm::mat4&& modelMatrix) noexcept :
        model_(model),
        vp_(vp),
        view_(view),
        mm_(modelMatrix) {
    }

    void render() {
        model_.render(
            vp_ * mm_, view_ * mm_, glm::transpose(glm::inverse(mm_)));
    }
};

}  // namespace neat
