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

#include <Binder.hh>
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

}  // namespace

namespace neat {

Billboard::Billboard(Billboard&& rhs) noexcept :
    vertices_(std::move(rhs.vertices_)), buffer_(std::move(rhs.buffer_)) {
}

Billboard::Billboard(glm::vec4* vertices) noexcept : vertices_(vertices) {
    if (!program_) {
        program_ = Program(
            {{GL_FRAGMENT_SHADER, billBoardF}, {GL_VERTEX_SHADER, billBoardV}});
    }

    Binder bufferBinder(buffer_);
    buffer_.set(vertices_, sizeof(glm::vec4) * 6);
    glEnableVertexAttribArray(0);
}

void Billboard::render(const Texture& texture) {
    Binder textureBinder(texture);
    Binder bufferBinder(buffer_);
    program_->use();
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

}  // namespace neat
