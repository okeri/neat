// Copyright 2023 Keri Oleg

#include <optional>
#include <GLES3/gl3.h>
#include <glm/gtc/type_ptr.hpp>

#include "App.hh"

App::App(int width, int height, const char* filename) :
    model_(filename),
    far_(60.f),
    pos_(glm::rotate(glm::mat4(1.f), 70.f, {-1, 0, 0})),
    projection_(glm::perspective(glm::radians(90.0f),
        static_cast<float>(width) / height, 0.1f, 4000.0f)) {
    if (!model_.valid()) {
        throw std::runtime_error("cannot load model!!!");
    }

    neat::Model::setLight(0, {-1.f, 1.f, 2.f}, {0.8f, 0.4f, 0.8f}, 0.f);
    model_.setPos(pos_);
    updateView(far_);
}

void App::updateView(float farDiff) {
    far_ += farDiff;
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.f, 0.0f, far_), glm::vec3(0, 40.f, 0), glm::vec3(0, 1, 0));
    neat::Model::setVP(view, projection_);
}

int App::action(neat::Actions action, int x, int y) {  // NOLINT
    static std::optional<std::pair<int, int>> pressed = std::nullopt;
    static glm::mat4 movePos = pos_;
    static constexpr auto speed = 100.f;

    switch (action) {
        case neat::Actions::Back:
            return 1;
        case neat::Actions::TouchDown:
            pressed = std::make_pair(x, y);
            break;
        case neat::Actions::TouchUp:
            pressed = std::nullopt;
            pos_ = movePos;
            break;
        case neat::Actions::Move:
            if (pressed) {
                movePos = glm::rotate(pos_,
                    static_cast<float>(x - pressed->first) / speed, {0, 0, 1});
                movePos = glm::rotate(movePos,
                    static_cast<float>(y - pressed->second) / speed, {1, 0, 0});
                model_.setPos(movePos);
            }
            break;

        case neat::Actions::Zoom:
            updateView(-y);
            break;

        default:
            break;
    }
    return 0;
}

void App::draw() {
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    model_.render();
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void App::update(unsigned iters) {
}
