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
