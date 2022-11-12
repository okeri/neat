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
    [[nodiscard]] unsigned int uniform(const char* name) noexcept;
    [[nodiscard]] unsigned int getRawId() const noexcept;
    void use() const noexcept;
};

}  // namespace neat
