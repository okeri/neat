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
#include <glm/gtc/type_ptr.hpp>

#include <Binder.hh>
#include <Text.hh>
#include <Program.hh>

namespace neat {

namespace {

// clang-format off

const char* textV = GLSL(
in vec4 pos_uv;
out vec2 uv;

void main() {
    gl_Position = vec4(pos_uv.xy, 0, 1);
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

Text::Entry::Entry(std::string_view t, float x, float y) :
    start(x, y), text(t) {
}

Text::Text(std::vector<Entry>&& e, const Font& font) : font_(font), values_(e) {
    std::vector<glm::vec4> data;
    for (const auto& entry : values_) {
        auto vertexes = font_.calculate(entry.text, entry.start);
        data.insert(data.end(), vertexes.begin(), vertexes.end());
    }

    Binder fontBinder(font_);

    if (!program_) {
        program_ =
            Program({{GL_FRAGMENT_SHADER, textF}, {GL_VERTEX_SHADER, textV}});
    }

    program_->use();
    count_ = data.size();

    Binder bufferBinder(buffer_);
    buffer_.set(data.data(), sizeof(glm::vec4) * count_);
    glEnableVertexAttribArray(0);
}

void Text::render() noexcept {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Binder fontBinder(font_);
    Binder bufferBinder(buffer_);
    program_->use();
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_TRIANGLES, 0, count_);
    glDisable(GL_BLEND);
}

size_t Text::count() const {
    return values_.size();
}

const glm::vec2& Text::operator[](unsigned index) const {
    return values_[index].start;
}

void Text::setColor(unsigned int color) {
    program_->use();
    glUniform4f(program_->uniform("inputColor"), color >> 16U,
        (color >> 8U) & 0xff, color & 0xff, 1);
}

void Text::draw(std::string_view text, const Font& font, float x, float y) {
    auto data = font.calculate(text, glm::vec2(x, y));

    Binder fontBinder(font);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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

}  // namespace neat
