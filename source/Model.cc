// Copyright 2018 Keri Oleg

#include <cstdint>
#include <string>
#include <unordered_map>
#include <functional>
#include <optional>
#include <filesystem>

#include <GLES3/gl3.h>
#include <glm/gtc/type_ptr.hpp>

#include <Log.hh>
#include <Asset.hh>
#include <Model.hh>
#include <Texture.hh>
#include <m3d.hh>
#include <Program.hh>
#include <Buffer.hh>
#include <Binder.hh>

namespace neat {

namespace {

// clang-format off
const char* modelV = GLSL(
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;
out vec2 uv;

uniform mat4 MVP;

void main() {
    gl_Position = MVP * vec4(vertex, 1.0);
    uv = texcoord;
}
);

const char* modelF = GLSL(
in vec2 uv;

uniform sampler2D texture;
layout (location = 0) out vec4 color;

void main() {
    color = texture2D(texture, uv);  
}
);

// clang-format on

class M3dStream : public Asset {
    uint32_t mainLength_ = 0;

  public:
#pragma pack(push, 1)
    struct Header {
        uint16_t id;
        uint32_t len;
    };
#pragma pack(pop)

    M3dStream(std::string_view filename) noexcept : Asset(filename) {
        Header header;
        if (Asset::valid() && read(&header, sizeof(Header)) != Error &&
            header.id == m3d::Chunk::Main) {
            mainLength_ = header.len;
        }
    }

    uint32_t length() const {
        return mainLength_;
    }

    template <class Callback>
    uint32_t visitChunks(uint32_t length, Callback callback) {
        Header header;
        uint32_t result = 0;
        while (result < length && read(&header, sizeof(Header)) != Error) {
            if (!callback(header)) {
                seek(header.len - sizeof(Header));
            }
            result += header.len + sizeof(Header);
        }
        return result;
    }

    std::string readChars(uint32_t len) {
        std::string result(len, '\0');
        result.erase();
        char sym = '\1';
        for (unsigned i = 0; i < len && sym != '\0'; ++i) {
            read(&sym, sizeof(sym));
            result += sym;
        }
        return result;
    }

    std::string readFilenameValue(uint32_t len) {
        std::string result;

        visitChunks(len, [this, &result](const Header& header) {
            if (header.id == m3d::Chunk::ReflectionMapFilename) {
                result = readChars(header.len);
                return true;
            }
            return false;
        });
        return result;
    }

    glm::vec3 readVecValue(uint32_t len) {
        glm::vec3 result;
        visitChunks(len, [this, &result](const Header& header) {
            switch (header.id) {
                case m3d::Chunk::Float:
                case m3d::Chunk::FloatGamma:
                    read(&result, sizeof(glm::vec3));
                    return true;

                case m3d::Chunk::Byte:
                case m3d::Chunk::ByteGamma: {
                    glm::vec<3, uint8_t> byteVec;
                    read(&byteVec, sizeof(byteVec));
                    result = glm::vec3(byteVec.x / 255.f, byteVec.y / 255.f,
                        byteVec.z / 255.f);
                }
                    return true;

                default:
                    return false;
            }
        });
        return result;
    }

    float readPercentValue(uint32_t len) {
        float result;
        visitChunks(len, [this, &result](const Header& header) {
            switch (header.id) {
                case m3d::Chunk::PercentInt: {
                    uint16_t value;
                    read(&value, sizeof(uint16_t));
                    result = static_cast<float>(value);
                }
                    return true;

                case m3d::Chunk::PercentFloat:
                    read(&result, sizeof(float));
                    return true;

                default:
                    return false;
            }
        });
        return result / 100.f;
    }
};

using Face = uint16_t;
using Faces = std::vector<glm::vec<3, Face>>;

class Material : private NoCopy {
    Buffer ibo_;
    unsigned faceCount_;

  public:
    glm::vec3 diffuse;
    glm::vec3 ambient;
    glm::vec3 specular;
    float specularExponent;
    std::optional<Texture> texture;

    Material() : faceCount_(0) {
    }

    Material(Material&& rhs) {
        diffuse = rhs.diffuse;
        ambient = rhs.ambient;
        specular = rhs.specular;
        specularExponent = rhs.specularExponent;
        ibo_ = std::move(rhs.ibo_);
        if (rhs.texture) {
            texture = std::move(*rhs.texture);
        }
    }

    Material& operator=(Material&& rhs) {
        diffuse = rhs.diffuse;
        ambient = rhs.ambient;
        specular = rhs.specular;
        specularExponent = rhs.specularExponent;
        if (rhs.texture) {
            texture = std::move(*rhs.texture);
        }
        return *this;
    }

    unsigned faceCount() const {
        return faceCount_;
    }

    void set(const std::vector<Face>& faces) {
        ibo_.bind(Buffer::Target::ElementArray);
        faceCount_ = faces.size();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(Face),
            faces.data(), GL_STATIC_DRAW);
        ibo_.unbind(Buffer::Target::ElementArray);
    }

    void bind() const {
        ibo_.bind(Buffer::Target::ElementArray);
        if (texture) {
            texture->bind();
        }
    }

    void unbind() const {
        ibo_.unbind(Buffer::Target::ElementArray);
        if (texture) {
            texture->unbind();
        }
    }
};

using Materials = std::vector<Material>;

struct MeshData : private NoCopy {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texcoords;
    std::vector<uint32_t> faceSmooth;
    Faces faces;

    bool valid() const {
        return (!vertices.empty() && !faces.empty());
    }
};

class Mesh : private GLResource {
    enum BufferType : unsigned { Vertexes, Normals, TexCoords, Count };

    std::array<Buffer, BufferType::Count> buffers_;
    const Materials& materials_;
    inline static std::optional<Program> program_;

  public:
    Mesh(Mesh&& rhs) noexcept :
        buffers_(std::move(rhs.buffers_)),
        materials_(rhs.materials_) {
        swap(std::move(rhs));
    }

    Mesh(const MeshData& data, const Materials& materials) noexcept :
        materials_(materials) {
        std::vector<glm::vec3> normals(
            data.vertices.size(), glm::vec3(0, 0, 0));

        glm::vec3 faceNormal;
        for (const auto& face : data.faces) {
            faceNormal =
                glm::cross(data.vertices[face.y] - data.vertices[face.x],
                    data.vertices[face.z] - data.vertices[face.x]);
            normals[face.x] += faceNormal;
            normals[face.y] += faceNormal;
            normals[face.z] += faceNormal;
        }

        for (auto& normal : normals) {
            normal = glm::normalize(normal);
        }

        glGenVertexArrays(1, &id_);
        glBindVertexArray(id_);

        glEnableVertexAttribArray(Vertexes);
        buffers_[Vertexes].bind();
        buffers_[Vertexes].set(data.vertices);
        glVertexAttribPointer(Vertexes, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(Normals);
        buffers_[Normals].bind();
        buffers_[Normals].set(normals);
        glVertexAttribPointer(Normals, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(TexCoords);
        buffers_[TexCoords].bind();
        buffers_[TexCoords].set(data.texcoords);
        glVertexAttribPointer(TexCoords, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindVertexArray(0);

        if (!program_) {
            program_ = Program(
                {{GL_FRAGMENT_SHADER, modelF}, {GL_VERTEX_SHADER, modelV}});
        }
    }

    void render(const glm::mat4& mvp) const {
        glEnable(GL_DEPTH_TEST);
        program_->use();
        glUniformMatrix4fv(
            program_->uniform("MVP"), 1, false, glm::value_ptr(mvp));

        glDisable(GL_BLEND);

        glBindVertexArray(id_);
        for (const auto& material : materials_) {
            Binder matBinder(material);
            glDrawElements(
                GL_TRIANGLES, material.faceCount(), GL_UNSIGNED_SHORT, nullptr);
        }
        glBindVertexArray(0);
    }

    ~Mesh() {
        if (id_ != 0) {
            glDeleteVertexArrays(1, &id_);
        }
    }
};

}  // namespace

class Model::Impl {
    std::vector<Mesh> meshes_;
    Materials materials_;

  public:
    Impl(std::string_view filename, float scale) noexcept {
        auto parentPath = std::filesystem::path(filename).parent_path();
        M3dStream source(filename);
        if (source.length() != 0) {
            auto handleMaterialBlock = [&source, &parentPath, this](
                                           uint32_t len) {
                Material material;
                float shininess;
                source.visitChunks(len, [&source, &parentPath, &material,
                                            &shininess](
                                            const M3dStream::Header& header) {
                    switch (header.id) {
                        case m3d::Chunk::TextureMap: {
                            auto filename =
                                source.readFilenameValue(header.len);
                            std::vector<uint8_t> buffer;
                            Asset asset(parentPath.append(filename).string());
                            if (asset.read(buffer) != Asset::Error) {
                                material.texture = Texture(
                                    Image(buffer.data(), buffer.size()));
                            } else {
                                Log() << "cannot load texture " << filename;
                            }
                        }
                            return true;

                        case m3d::Chunk::AmbientColor:
                            material.ambient = source.readVecValue(header.len);
                            return true;

                        case m3d::Chunk::DiffuseColor:
                            material.diffuse = source.readVecValue(header.len);
                            return true;

                        case m3d::Chunk::SpecularColor:
                            material.specular = source.readVecValue(header.len);
                            return true;

                        case m3d::Chunk::SpecularExponent:
                            material.specularExponent =
                                source.readPercentValue(header.len) * 128.f;
                            return true;

                        case m3d::Chunk::Shininess:
                            shininess = source.readPercentValue(header.len);
                            return true;

                        default:
                            return false;
                    }
                });
                material.specular *= shininess;
                materials_.push_back(std::move(material));
                return true;
            };

            auto handleMeshBlock = [&source, scale, this](uint32_t len) {
                MeshData mesh;
                std::vector<std::vector<Face>> materialFaces;

                auto handleFaces = [&source, &mesh, &materialFaces](
                                       const M3dStream::Header& header) {
                    uint16_t count;
                    switch (header.id) {
                        case m3d::Chunk::FacesMaterial: {
                            source.readChars(header.len);
                            source.read(&count, sizeof(uint16_t));
                            auto faces = materialFaces.emplace_back(count);
                            source.read(faces.data(), sizeof(Face) * count);
                        }
                            return true;

                        case m3d::Chunk::SmoothGroup:
                            source.read(&count, sizeof(uint16_t));
                            mesh.faceSmooth.resize(count);
                            source.read(mesh.faceSmooth.data(),
                                count * sizeof(uint32_t));
                            return true;

                        default:
                            return false;
                    }
                };

                source.visitChunks(len, [&mesh, &handleFaces, &source, scale](
                                            const M3dStream::Header& header) {
                    uint16_t count;
                    switch (header.id) {
                        case m3d::Chunk::Vertices:
                            source.read(&count, sizeof(uint16_t));
                            mesh.vertices.resize(count);
                            source.read(mesh.vertices.data(),
                                sizeof(glm::vec3) * count);
                            for (auto& v : mesh.vertices) {
                                v *= scale;
                            }

                            return true;

                        case m3d::Chunk::FacesDescr:
                            source.read(&count, sizeof(uint16_t));
                            mesh.faces.reserve(count);
                            for (auto i = 0; i < count; ++i) {
                                glm::vec<3, Face> face;
                                source.read(&face, sizeof(face));
                                mesh.faces.push_back(face);
                                source.seek(2);  // skip flags
                            }
                            source.visitChunks(
                                header.len - sizeof(uint16_t) -
                                    (count * (sizeof(glm::vec<3, Face>) + 2)),
                                handleFaces);
                            return true;

                        case m3d::Chunk::MappingCoords:
                            source.read(&count, sizeof(uint16_t));
                            mesh.texcoords.resize(count);
                            source.read(mesh.texcoords.data(),
                                count * sizeof(glm::vec2));
                            return true;

                        default:
                            return false;
                    }
                });

                if (mesh.valid()) {
                    for (size_t mat = 0; mat != materialFaces.size(); ++mat) {
                        std::vector<Face> result;
                        for (size_t face = 0; face != materialFaces[mat].size();
                             ++face) {
                            result.push_back(mesh.faces[face].x);
                            result.push_back(mesh.faces[face].y);
                            result.push_back(mesh.faces[face].z);
                        }
                        materials_[mat].set(result);
                    }
                    meshes_.emplace_back(mesh, materials_);
                }
                return true;
            };

            source.visitChunks(source.length(), [&source, handleMaterialBlock,
                                                    handleMeshBlock](
                                                    const M3dStream::Header&
                                                        header) {
                if (header.id == m3d::Chunk::Editor) {
                    source.visitChunks(header.len,
                        [&source, handleMaterialBlock, handleMeshBlock](
                            const M3dStream::Header& header) {
                            switch (header.id) {
                                case m3d::Chunk::ObjectBlock: {
                                    auto read = source.readChars(header.len);
                                    source.visitChunks(
                                        header.len - read.length() - 1,
                                        [handleMeshBlock](
                                            const M3dStream::Header& header) {
                                            if (header.id == m3d::Chunk::Mesh) {
                                                return handleMeshBlock(
                                                    header.len);
                                            }
                                            return false;
                                        });
                                }
                                    return true;

                                case m3d::Chunk::MaterialBlock:
                                    return handleMaterialBlock(header.len);

                                default:
                                    return false;
                            }
                        });
                    return true;
                }
                return false;
            });
        }
    }

    void render(const glm::mat4& mvp) const {
        for (const auto& mesh : meshes_) {
            mesh.render(mvp);
        }
    }

    bool valid() const {
        return !meshes_.empty();
    }

    ~Impl() {
    }
};

Model::Model(std::string_view filename, float scale) noexcept :
    pImpl_(std::make_unique<Impl>(filename, scale)) {
}

Model::Model(Model&& rhs) noexcept : pImpl_(std::move(rhs.pImpl_)) {
}

Model::~Model() {
}

void Model::render(const glm::mat4& mvp) const {
    pImpl_->render(mvp);
}

bool Model::valid() const {
    return pImpl_->valid();
}

}  // namespace neat
