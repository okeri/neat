// Copyright 2023 Keri Oleg

#pragma once

#include <stdexcept>

#include <NoCopy.hh>
#include <Actions.hh>
#include <Model.hh>

#include <glm/mat4x4.hpp>

class App : private neat::NoCopy {
    neat::Model model_;
    glm::mat4 pos_;

  public:
    App(int width, int height, const char* filename);
    ~App() = default;
    int action(neat::Actions act, int x, int y);
    void draw();
    void update(unsigned iters);
};