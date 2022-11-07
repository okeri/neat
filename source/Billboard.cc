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

#include <GLES3/gl3.h>

#include <array>

#include <glm/vec4.hpp>

#include <Program.hh>
#include <Billboard.hh>

namespace {

// clang-format off

const char* billBoardV = GLSL(
in vec4 position;
out vec2 uv;

void main() {
    uv = vec2(position.z, position.w);
    gl_Position = vec4(position.x, position.y, 0, 1);
}
);

const char* billBoardF = GLSL(
in vec2 uv;
out vec4 outColor;

uniform sampler2D frame;
void main() {
    outColor = texture(frame, uv);
}
);

// clang-format on

std::array<glm::vec4, 6> billBoardVertices(const glm::vec4& rect) {
    return {{{rect.x, rect.w, 0., 1.}, {rect.x, rect.y, 0., 0.},
        {rect.z, rect.y, 1., 0.}, {rect.x, rect.w, 0., 1.},
        {rect.z, rect.y, 1., 0.}, {rect.z, rect.w, 1., 1}}};
}

class Blending : private neat::NoCopy {
  public:
    Blending() noexcept {
        glEnable(GL_BLEND);
    }
    ~Blending() noexcept {
        glDisable(GL_BLEND);
    }
};

}  // namespace

namespace neat {

Billboard::Billboard(const glm::vec4& rect) noexcept {
    initProgram();
    buffer_.bind();
    auto vertices = billBoardVertices(rect);
    buffer_.set(vertices.data(), sizeof(glm::vec4) * 6);
    glEnableVertexAttribArray(0);
}

Billboard::Billboard(Billboard&& rhs) noexcept :
    buffer_(std::move(rhs.buffer_)) {
}

void Billboard::render(const Texture& texture) const noexcept {
    Blending blenging;
    texture.bind();
    buffer_.bind();
    program_->use();
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Billboard::draw(const glm::vec4& rect, const Texture& texture) {
    Blending blenging;
    texture.bind();
    program_->use();
    auto vertices = billBoardVertices(rect);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, vertices.data());
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Billboard::initProgram() {
    if (!program_) {
        program_ = Program(
            {{GL_FRAGMENT_SHADER, billBoardF}, {GL_VERTEX_SHADER, billBoardV}});
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

}  // namespace neat
