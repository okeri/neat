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

#include <stdexcept>

#include <NoCopy.hh>
#include <Actions.hh>
#include <Model.hh>

#include <glm/mat4x4.hpp>

class App : private neat::NoCopy {
    neat::Model model_;
    float far_;
    glm::mat4 pos_;
    glm::mat4 projection_;
    void updateView(float farDiff);

  public:
    App(int width, int height, const char* filename);
    ~App() = default;
    int action(neat::Actions act, int x, int y);
    void draw();
    void update(unsigned iters);
};
