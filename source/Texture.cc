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

Texture::Texture(
    const void* data, unsigned bpp, unsigned width, unsigned height) noexcept {
    if (bpp == 0 || bpp > 4) {
        return;
    }
    glGenTextures(1, &id_);
    bind();
    if (bpp != 4) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }
    GLint formats[] = {GL_RED, GL_RG, GL_RGB, GL_RGBA};
    glTexImage2D(GL_TEXTURE_2D, 0, formats[bpp - 1], width, height, 0,
        formats[bpp - 1], GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::Texture(const Image& image) noexcept :
    Texture(image.data(), image.bpp(), image.width(), image.height()) {
}

Texture::Texture(Texture&& rhs) noexcept : GLResource(std::move(rhs)) {
}

Texture& Texture::operator=(Texture&& rhs) noexcept {
    GLResource::operator=(std::move(rhs));
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
