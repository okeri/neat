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

#include <functional>
#include <memory>

#include <GLES3/gl3.h>

#include <Log.hh>
#include <Program.hh>

namespace neat {

namespace {

using AttachFunc = std::function<void(GLuint, GLuint)>;
using AttachWraper = std::function<void(AttachFunc, GLuint)>;

GLuint compile(GLenum shaderType, const char* src) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

        GLint size;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
        if (size) {
            std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
            glGetShaderInfoLog(shader, size, nullptr, buffer.get());
            Log() << buffer.get();
        }

        if (!compiled) {
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

GLuint program(AttachWraper wrapper) {
    GLuint program = glCreateProgram();
    if (program) {
        wrapper(glAttachShader, program);
        glLinkProgram(program);

        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint size;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
            if (size) {
                std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
                glGetProgramInfoLog(program, size, nullptr, buffer.get());
                Log() << buffer.get();
            }
            wrapper(glDetachShader, program);
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

}  // namespace

Program::Program(const std::vector<ShaderInfo>& shaders) noexcept {
    std::vector<GLuint> compiled;
    for (const auto& shader : shaders) {
        compiled.emplace_back(compile(shader.type, shader.source));
    }
    id_ = program([compiled](AttachFunc func, GLuint p) {
        for (const auto& c : compiled) {
            func(p, c);
        }
    });
}

Program::Program(Program&& rhs) noexcept : GLResource(std::move(rhs)) {
}

Program& Program::operator=(Program&& rhs) noexcept {
    GLResource::operator=(std::move(rhs));
    return *this;
}

void Program::use() const noexcept {
    glUseProgram(id_);
}

unsigned int Program::uniform(const char* name) noexcept {
    auto found = uniforms_.find(name);
    if (found != uniforms_.end()) {
        return found->second;
    }
    auto result = glGetUniformLocation(id_, name);
    uniforms_[name] = result;
    if (result == GL_INVALID_VALUE) {
        Log() << "cant get uniform " << name;
    }
    return result;
}

unsigned int Program::getRawId() const noexcept {
    return id_;
}

}  // namespace neat
