// Copyright 2018 Keri Oleg

#include <optional>

#include <GLES3/gl3.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <Buffer.hh>
#include <Texture.hh>
#include <Font.hh>

namespace neat {

struct FontCommon : private NoCopy {
    FT_Library lib;

    FontCommon() {
        FT_Init_FreeType(&lib);
    }

    ~FontCommon() {
        FT_Done_FreeType(lib);
    }
};

class Font::Impl {
    struct CharInfo {
        float left;
        float top;
        float advanceX;
        float advanceY;
        float width;
        float height;
        float uvWidth;
        float uvHeight;
        float uvOffset;
    };

    constexpr static unsigned charStart = 0x20;
    constexpr static unsigned charEnd = 0x7f;
    constexpr static unsigned charCount = charEnd - charStart;
    constexpr static float scale_ = 0.0025;

    static inline FontCommon common_;

    float width_;
    float height_;
    std::optional<Texture> atlas_;
    CharInfo chars_[charCount];
    bool valid_;

    void copyToAtlas(unsigned int xOfs, unsigned int yOfs, unsigned int width,
        unsigned int height, void* buffer) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, xOfs, yOfs, width, height, GL_RED,
            GL_UNSIGNED_BYTE, buffer);
    }

  public:
    Impl(const void* data, std::size_t size, int height) noexcept :
        valid_(false) {
        FT_Face face;
        auto error = FT_New_Memory_Face(
            common_.lib, static_cast<const FT_Byte*>(data), size, 0, &face);
        if (!error) {
            FT_Set_Pixel_Sizes(face, 0, height);
            valid_ = true;
        }

        float fixedWidth = 0;
        height_ = 0;
        FT_GlyphSlot glyph = face->glyph;
        for (unsigned i = 0; i < charCount; i++) {
            if (!FT_Load_Char(face, i + charStart, FT_LOAD_RENDER)) {
                if (glyph->bitmap.rows > height_) {
                    height_ = glyph->bitmap.rows;
                }
                if (glyph->bitmap.width > fixedWidth) {
                    fixedWidth = glyph->bitmap.width;
                }
            }
        }

        width_ = fixedWidth * charCount;
        atlas_ = Texture(1, width_, height_);

        int x = 0;
        for (unsigned i = 0; i < charCount; i++) {
            if (!FT_Load_Char(face, i + charStart, FT_LOAD_RENDER)) {
                chars_[i].advanceX = (glyph->advance.x >> 6) * scale_;
                chars_[i].advanceY = (glyph->advance.y >> 6) * scale_;
                chars_[i].width = glyph->bitmap.width * scale_;
                chars_[i].height = glyph->bitmap.rows * scale_;
                chars_[i].left = glyph->bitmap_left * scale_;
                chars_[i].top = glyph->bitmap_top * scale_;
                chars_[i].uvOffset = x / width_;
                chars_[i].uvWidth = glyph->bitmap.width / width_;
                chars_[i].uvHeight = glyph->bitmap.rows / height_;
                copyToAtlas(x, 0, glyph->bitmap.width, glyph->bitmap.rows,
                    glyph->bitmap.buffer);
            } else {
                chars_[i].width = 0;
            }
            x += fixedWidth;
        }

        if (valid_) {
            FT_Done_Face(face);
        }
    }

    Impl(Impl&& rhs) noexcept :
        width_(rhs.width_),
        height_(rhs.height_),
        atlas_(std::move(rhs.atlas_)),
        valid_(rhs.valid_) {
        std::copy(rhs.chars_, rhs.chars_ + charCount, chars_);
        rhs.valid_ = false;
    }

    std::vector<glm::vec4> calculate(
        std::string_view text, const glm::vec2& coords) const {
        std::vector<glm::vec4> result(6 * text.length());
        int n = 0;
        float x = coords.x;
        float y = coords.y;

        for (const auto& p : text) {
            const auto& sym = chars_[p - charStart];

            float x2 = x + sym.left;
            float y2 = y - sym.top;
            x += sym.advanceX;
            y += sym.advanceY;

            result[n++] = glm::vec4(x2, -y2, sym.uvOffset, 0.);
            result[n++] =
                glm::vec4(x2 + sym.width, -y2, sym.uvOffset + sym.uvWidth, 0.);
            result[n++] =
                glm::vec4(x2, -y2 - sym.height, sym.uvOffset, sym.uvHeight);
            result[n++] =
                glm::vec4(x2 + sym.width, -y2, sym.uvOffset + sym.uvWidth, 0.);
            result[n++] =
                glm::vec4(x2, -y2 - sym.height, sym.uvOffset, sym.uvHeight);
            result[n++] = glm::vec4(x2 + sym.width, -y2 - sym.height,
                sym.uvOffset + sym.uvWidth, sym.uvHeight);
        }
        return result;
    }

    float width(std::string_view str) const {
        float result = 0;
        for (auto c : str) {
            auto i = c - charStart;
            result += chars_[i].advanceX + chars_[i].left + chars_[i].width;
        }
        return result;
    }

    float height() const {
        return height_ * scale_;
    }

    float centerX(std::string_view str) const {
        return width(str) / -4;
    }

    bool valid() const {
        return valid_;
    }

    void bind() const {
        atlas_->bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    void unbind() const {
        atlas_->unbind();
    }
};

Font::Font(const void* data, std::size_t size, int height) noexcept :
    pImpl_(data, size, height) {
}

Font::Font(Font&& rhs) noexcept : pImpl_(std::move(rhs.pImpl_)) {
}

std::vector<glm::vec4> Font::calculate(
    std::string_view text, const glm::vec2& coords) const {
    return pImpl_->calculate(text, coords);
}

void Font::bind() const {
    pImpl_->bind();
}

void Font::unbind() const {
    pImpl_->unbind();
}

float Font::centerX(std::string_view str) const {
    return pImpl_->centerX(str);
}

float Font::height() const {
    return pImpl_->height();
}

float Font::width(std::string_view str) const {
    return pImpl_->width(str);
}

bool Font::valid() const {
    return pImpl_->valid();
}

Font::~Font() {
}

}  // namespace neat
