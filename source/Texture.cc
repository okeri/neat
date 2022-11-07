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

#include <utility>

#include <GLES3/gl3.h>

#include <Texture.hh>

namespace neat {

Texture::Texture(unsigned bpp, unsigned width, unsigned height) noexcept {
    glGenTextures(1, &id_);
    bind();
    auto format = bpp == 1 ? GL_RED : GL_RGBA8;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
        GL_UNSIGNED_BYTE, nullptr);
}

Texture::Texture(const Image& image) noexcept {
    glGenTextures(1, &id_);
    bind();
    if (!image.alpha()) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, image.alpha() ? GL_RGBA8 : GL_RGB8,
        image.width(), image.height(), 0, image.alpha() ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE, image.data());
    glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::Texture(Texture&& rhs) noexcept {
    swap(std::move(rhs));
}

Texture& Texture::operator=(Texture&& rhs) noexcept {
    swap(std::move(rhs));
    return *this;
}

Texture::~Texture() {
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
    }
}

void Texture::bind() const noexcept {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id_);
}

void Texture::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace neat
