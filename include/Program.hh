// Copyright 2018 Keri Oleg

#pragma once

#define GLSL(_X_) "#version 320 es\nprecision mediump float;\n" #_X_

#include <string>
#include <vector>
#include <unordered_map>

#include "GLResource.hh"

namespace neat {

struct ShaderInfo {
    unsigned int type;
    const char* source;
};

class Program : private GLResource {
    std ::unordered_map<std::string, unsigned int> uniforms_;

  public:
    explicit Program(const std::vector<ShaderInfo>& shaders) noexcept;
    Program(Program&& rhs) noexcept;
    Program& operator=(Program&& rhs) noexcept;
    unsigned int uniform(const char* name);
    void use();
    unsigned int getRawId() const;
};

}  // namespace neat
