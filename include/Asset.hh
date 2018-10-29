// Copyright 2018 Keri Oleg

#include <string_view>
#include <vector>

#pragma once

#ifdef ANDROID
#include <android/asset_manager.h>
#else
#include <fstream>
#endif

namespace neat {

class Asset {
#ifdef ANDROID
    AAsset* file_;
#else
    std::ifstream file_;
#endif

  public:
    enum { Error = 0xffffffffffffffff };

    Asset(std::string_view filename) noexcept;
    ~Asset();
    bool valid();
    bool open(std::string_view filename);
    void close();
    uint64_t read(void* data, uint64_t size);
    uint64_t read(std::vector<uint8_t>& data);
    uint64_t seek(uint64_t ofs);

#ifdef ANDROID
    static void setManager(AAssetManager* manager);
#endif
};

}  // namespace neat
