// Copyright 2018 Keri Oleg

#pragma once

#include <cstdint>

namespace neat {

class Image {
    uint8_t* data_;
    std::size_t size_;
    uint32_t width_;
    uint32_t height_;

    void load(const void* data, std::size_t size) noexcept;

  public:
    Image(const void* data, std::size_t size, bool vflip = false) noexcept;
    Image(Image&& other) noexcept;
    uint8_t* data() const;
    uint32_t width() const;
    uint32_t height() const;
    bool valid();
    ~Image();
};

}  // namespace neat
