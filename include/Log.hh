// Copyright 2018 Keri Oleg

#pragma once

#include <sstream>
#include "NoCopy.hh"

namespace neat {

class Log : private NoCopy {
    std::ostringstream message_;

  public:
    Log() = default;
    ~Log();

    template <typename T>
    Log& operator<<(const T& obj) {
        message_ << obj;
        return *this;
    }
};

}  // namespace neat
