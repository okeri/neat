// Copyright 2018 Keri Oleg

#pragma once

#include "NoCopy.hh"

namespace neat {

/// Underlying class must have implementation of methods bind(...) and unbind()

template <class T, typename... BindArgs>
class Binder : private NoCopy {
    T& t_;

  public:
    explicit Binder(T& t, BindArgs... args) : t_(t) {
        t_.bind(args...);
    }

    ~Binder() {
        t_.unbind();
    }
};

}  // namespace neat
