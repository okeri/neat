// Copyright 2018 Keri Oleg

#include <GLES3/gl3.h>

#include <utility>

#include <Texture.hh>

namespace neat {

Texture::Texture(unsigned bpp, unsigned width, unsigned height) noexcept {
    auto getFormat = [bpp]() {
        switch (bpp) {
            case 1:
                return GL_RED;

            default:
                return GL_RGB8;
        }
    };

    glGenTextures(1, &id_);
    bind();
    auto format = getFormat();
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
        GL_UNSIGNED_BYTE, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

Texture::Texture(Texture&& rhs) noexcept {
    swap(std::move(rhs));
}

Texture& Texture::operator=(Texture&& rhs) noexcept {
    swap(std::move(rhs));
    return *this;
}

Texture::Texture(const Image& image) noexcept {
    glGenTextures(1, &id_);
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, image.width(), image.height(), 0,
        GL_RGB, GL_UNSIGNED_BYTE, image.data());
    glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::~Texture() {
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
    }
}

void Texture::bind() const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id_);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace neat
