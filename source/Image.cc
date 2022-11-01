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

#include <cstring>
#include <vector>

#include <png.h>

#include <Image.hh>

namespace {

struct PngBuf {
    const png_byte* data;
    size_t offset;
    size_t size;
};

void cbread(void* data, uint8_t* dst, size_t size) {
    auto* buffer =
        static_cast<PngBuf*>(png_get_io_ptr(static_cast<png_structp>(data)));
    memcpy(dst, buffer->data + buffer->offset, size);
    buffer->offset += size;
}

}  // namespace

namespace neat {

// NOLINTNEXTLINE:hicpp-member-init
Image::Image(const void* data, size_t size, bool vflip) noexcept {
    load(data, size);
    if (valid() && vflip) {
        auto* flipped = new uint8_t[size_];
        auto stride = size_ / height_;
        auto from = data_, to = flipped + size_ - stride;
        for (; to > flipped; from += stride, to -= stride) {
            std::copy(from, from + stride, to);
        }
        delete[] data_;
        data_ = flipped;
    }
}

Image::Image(Image&& other) noexcept {
    if (other.valid()) {
        data_ = other.data_;
        size_ = other.size_;
        width_ = other.width_;
        height_ = other.height_;
        other.size_ = 0;
    }
}

void Image::load(const void* data, size_t size) noexcept {
    if (size == 0) {
        return;
    }
    png_structp pngPtr = png_create_read_struct(
        PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop infoPtr = png_create_info_struct(pngPtr);

    PngBuf buffer{static_cast<const uint8_t*>(data), 0, size};
    png_set_read_fn(pngPtr, &buffer, reinterpret_cast<png_rw_ptr>(cbread));
    size_ = 0;
    if (setjmp(png_jmpbuf(pngPtr))) {
        png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
        return;
    }

    // read image header
    int bit_depth, fmt;
    png_read_info(pngPtr, infoPtr);
    png_get_IHDR(pngPtr, infoPtr, &width_, &height_, &bit_depth, &fmt, nullptr,
        nullptr, nullptr);

    if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(pngPtr);
    }

    if (fmt == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(pngPtr);
    }

    if (bit_depth == 16) {
        png_set_scale_16(pngPtr);
    }

    if (fmt == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(pngPtr);
    }

    if (fmt == PNG_COLOR_TYPE_RGBA) {
        png_set_strip_alpha(pngPtr);
    }

    if (bit_depth < 8) {
        png_set_packing(pngPtr);
    }
    png_read_update_info(pngPtr, infoPtr);

    // read data
    const png_size_t row_size = png_get_rowbytes(pngPtr, infoPtr);
    if (row_size) {
        size_ = row_size * height_;
        data_ = new uint8_t[size_];

        std::vector<png_bytep> row_ptrs(height_);

        png_uint_32 i;
        for (i = 0; i < height_; i++) {
            row_ptrs[i] = data_ + i * row_size;
        }
        png_read_image(pngPtr, row_ptrs.data());
    }

    // free
    png_read_end(pngPtr, infoPtr);
    png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
}

uint32_t Image::width() const noexcept {
    return width_;
}

uint32_t Image::height() const noexcept {
    return height_;
}

bool Image::valid() const noexcept {
    return size_ != 0;
}

Image::~Image() {
    if (valid()) {
        delete[] data_;
    }
}

uint8_t* Image::data() const noexcept {
    return data_;
}

}  // namespace neat
