// Copyright 2018 Keri Oleg

#pragma once

#include "NoCopy.hh"

namespace neat {

class GLResource : private NoCopy {
  protected:
    unsigned int id_;
    GLResource();

  public:
    void swap(GLResource&& other) noexcept;
};

}  // namespace neat
