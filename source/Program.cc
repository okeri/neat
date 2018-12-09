// Copyright 2018 Keri Oleg

#include <GLES3/gl3.h>
#include <functional>
#include <memory>

#include <Log.hh>
#include <Program.hh>

namespace neat {

namespace {

using AttachFunc = std::function<void(GLuint, GLuint)>;
using AttachWraper = std::function<void(AttachFunc, GLuint)>;

GLuint compile(GLenum shaderType, const char* src) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &src, NULL);
        glCompileShader(shader);
        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

        GLint size;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
        if (size) {
            std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
            glGetShaderInfoLog(shader, size, NULL, buffer.get());
            Log() << "Error: " << buffer.get();
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
                glGetProgramInfoLog(program, size, NULL, buffer.get());
                Log() << "Error: " << buffer.get();
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

Program::Program(Program&& rhs) noexcept {
    swap(std::move(rhs));
}

Program& Program::operator=(Program&& rhs) noexcept {
    swap(std::move(rhs));
    return *this;
}

void Program::use() {
    glUseProgram(id_);
}

unsigned int Program::uniform(const char* name) {
    auto found = uniforms_.find(name);
    if (found != uniforms_.end()) {
        return found->second;
    } else {
        auto result = glGetUniformLocation(id_, name);
        uniforms_[name] = result;
        if (result == GL_INVALID_VALUE) {
            Log() << "cant get uniform " << name;
        }
        return result;
    }
}

unsigned int Program::getRawId() const {
    return id_;
}

}  // namespace neat
