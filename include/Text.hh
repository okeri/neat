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

#include <vector>
#include <optional>
#include <glm/vec2.hpp>

#include "Font.hh"
#include "Buffer.hh"
#include "Program.hh"

namespace neat {

class Text : private NoCopy {
    Buffer buffer_;
    const Font& font_;
    unsigned count_;
    inline static std::optional<Program> program_;

  public:
    struct Entry {
        glm::vec2 start;
        std::string_view text;
        // TODO: bool visible;
      public:
        Entry(std::string_view text, float x, float y);
    };

  private:
    std::vector<Entry> values_;

  public:
    Text(std::vector<Entry>&&, const Font& font);
    void render();
    const glm::vec2& operator[](unsigned);
    size_t count() const;

    static void setColor(unsigned int color);
    static void draw(std::string_view text, const Font& font, float x, float y);
};

}  // namespace neat
