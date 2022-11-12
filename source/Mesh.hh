#pragma once

#include <array>

#include <glm/gtc/type_ptr.hpp>
#include <GLES3/gl3.h>

#include <Buffer.hh>

namespace neat {

class Mesh : private GLResource {
    enum BufferType : unsigned { Vertexes, Normals, TexCoords, Faces, Count };

    std::array<Buffer, BufferType::Count> buffers_;
    unsigned faceCount_;
    unsigned materialIndex_;

  public:
    using Face = uint32_t;

    Mesh(const void* vertices, const void* normals, const void* texcoords,
        unsigned count, const void* faces, unsigned faceCount,
        unsigned materialIndex) noexcept :
        faceCount_(faceCount), materialIndex_(materialIndex) {
        buffers_[Faces] = Buffer(Buffer::Target::ElementArray);
        glGenVertexArrays(1, &id_);
        glBindVertexArray(id_);
        glEnableVertexAttribArray(Vertexes);
        buffers_[Vertexes].bind();
        buffers_[Vertexes].set(vertices, count * sizeof(glm::vec3));
        glVertexAttribPointer(Vertexes, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glEnableVertexAttribArray(Normals);
        buffers_[Normals].bind();
        buffers_[Normals].set(normals, count * sizeof(glm::vec3));
        glVertexAttribPointer(Normals, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glEnableVertexAttribArray(TexCoords);
        buffers_[TexCoords].bind();
        buffers_[TexCoords].set(texcoords, count * sizeof(glm::vec2));
        glVertexAttribPointer(TexCoords, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glBindVertexArray(0);

        buffers_[Faces].bind();
        buffers_[Faces].set(faces, faceCount_ * sizeof(Face));
    }

    Mesh(Mesh&& rhs) noexcept :
        buffers_(std::move(rhs.buffers_)),
        faceCount_(rhs.faceCount_),
        materialIndex_(rhs.materialIndex_) {
    }

    [[nodiscard]] unsigned materialIndex() const noexcept {
        return materialIndex_;
    }

    void bind() const noexcept {
        glBindVertexArray(id_);
        buffers_[Faces].bind();
    }

    void render() const noexcept {
        glDrawElements(GL_TRIANGLES, faceCount_, GL_UNSIGNED_INT, nullptr);
    }

    static void unbind() noexcept {
        glBindVertexArray(0);
    }

    ~Mesh() {
        if (id_ != 0) {
            unbind();
            buffers_[Faces].unbind();
            glDeleteVertexArrays(1, &id_);
        }
    }
};

}  // namespace neat
