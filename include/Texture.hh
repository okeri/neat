// Copyright 2018 Keri Oleg

#pragma once

#include "Image.hh"
#include "GLResource.hh"

namespace neat {

class Texture : private GLResource {
  public:
    Texture(unsigned bpp, unsigned width, unsigned height) noexcept;
    Texture(Texture&& rhs) noexcept;
    Texture(const Image& image) noexcept;
    Texture& operator=(Texture&& rhs) noexcept;
    void bind() const;
    void unbind() const;
    ~Texture();
};

}  // namespace neat
