// Copyright 2018 Keri Oleg

#include <png.h>

#include <cstring>
#include <vector>

#include <Image.hh>

namespace {

typedef struct PngBuf {
    const png_byte* data;
    size_t offset;
    size_t size;
} * PngBufPtr;

void cbread(void* data, uint8_t* dst, size_t size) {
    PngBufPtr buffer =
        static_cast<PngBufPtr>(png_get_io_ptr(static_cast<png_structp>(data)));
    memcpy(dst, buffer->data + buffer->offset, size);
    buffer->offset += size;
}

}  // namespace

namespace neat {

Image::Image(const void* data, size_t size, bool vflip) noexcept {
    load(data, size);
    if (valid() && vflip) {
        uint8_t* flipped = new uint8_t[size_];
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
    png_structp pngPtr =
        png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop infoPtr = png_create_info_struct(pngPtr);

    PngBuf buffer{static_cast<const uint8_t*>(data), 0, size};
    png_set_read_fn(pngPtr, &buffer, reinterpret_cast<png_rw_ptr>(cbread));
    size_ = 0;
    if (setjmp(png_jmpbuf(pngPtr))) {
        png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
        return;
    }

    // read image header
    int bit_depth, fmt;
    png_read_info(pngPtr, infoPtr);
    png_get_IHDR(
        pngPtr, infoPtr, &width_, &height_, &bit_depth, &fmt, NULL, NULL, NULL);

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
    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
}

uint32_t Image::width() const {
    return width_;
}

uint32_t Image::height() const {
    return height_;
}

bool Image::valid() {
    return size_ != 0;
}

Image::~Image() {
    if (valid()) {
        delete[] data_;
    }
}

uint8_t* Image::data() const {
    return data_;
}

}  // namespace neat
