// Copyright 2018 Keri Oleg

#pragma once

namespace neat {

class NoCopy {
    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;
    NoCopy(NoCopy&&) = delete;
    NoCopy& operator=(NoCopy&&) = delete;

  protected:
    NoCopy() = default;
};

}  // namespace neat
