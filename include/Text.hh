// Copyright 2018 Keri Oleg

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
