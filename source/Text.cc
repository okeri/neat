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

#include <Text.hh>
#include <Program.hh>

namespace neat {

namespace {

// clang-format off

const char* textV = GLSL(
in vec4 pos_uv;
out vec2 uv;

uniform vec2 shift;

void main() {
    gl_Position = vec4(pos_uv.xy + shift, 0, 1);
    uv = pos_uv.zw;
}
);

const char* textF = GLSL(
in vec2 uv;
uniform sampler2D texture;
uniform vec4 inputColor;
out vec4 color;

void main() {
    color = vec4(inputColor.rgb, texture2D(texture, uv).r);
}
);

// clang-format on

}  // namespace

Text::Text(const Font& font) noexcept : font_(font) {
    glEnableVertexAttribArray(0);
    if (!program_) {
        program_ =
            Program({{GL_FRAGMENT_SHADER, textF}, {GL_VERTEX_SHADER, textV}});
    }

    program_->use();
    buffer_.bind();
}

Text::Text(std::string_view text, float x, float y, const Font& font) noexcept :
    Text(font) {
    auto vertexes = font_.calculate(text, x, y);
    buffer_.set(vertexes.data(), sizeof(glm::vec4) * vertexes.size());
}

Text::Text(const std::vector<Entry>& values, const Font& font) noexcept :
    Text(font) {
    std::vector<glm::vec4> data;
    for (const auto& entry : values) {
        auto vertexes = font_.calculate(entry.text, entry.x, entry.y);
        data.insert(data.end(), vertexes.begin(), vertexes.end());
    }
    buffer_.set(data.data(), sizeof(glm::vec4) * data.size());
}

Text::Text(Text&& other) noexcept :
    buffer_(std::move(other.buffer_)), font_(other.font_) {
}

void Text::render() const noexcept {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    font_.bind();
    buffer_.bind();
    program_->use();
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_TRIANGLES, 0, buffer_.size() / sizeof(glm::vec4));
    glDisable(GL_BLEND);
}

void Text::move(float x, float y) noexcept {
    program_->use();
    glUniform2f(program_->uniform("shift"), x, y);
}

void Text::setColor(unsigned int color) {
    program_->use();
    glUniform4f(program_->uniform("inputColor"), color >> 16U,
        (color >> 8U) & 0xff, color & 0xff, 1);
}

void Text::draw(std::string_view text, const Font& font, float x, float y) {
    auto data = font.calculate(text, x, y);
    Buffer::unbind(Buffer::Target::Array);
    font.bind();
    if (!program_) {
        program_ =
            Program({{GL_FRAGMENT_SHADER, textF}, {GL_VERTEX_SHADER, textV}});
    }

    program_->use();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, data.data());
    glDrawArrays(GL_TRIANGLES, 0, data.size());
    glDisable(GL_BLEND);
}

Text::Entry::Entry(std::string_view t, float startx, float starty) noexcept :
    x(startx), y(starty), text(t) {
}

}  // namespace neat
