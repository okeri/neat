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

struct Material {
    vec3 diffuse;
    vec3 ambient;
    vec3 specular;
};

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

out vec2 uv;
out vec3 fragColor;

uniform mat4 MVP;
uniform mat4 MV;
uniform mat4 NM;

uniform Material material;

const int maxLightCount = 16;
uniform int lightCount;

struct Light {
    vec3 position;
    vec3 color;
};

uniform Light lights[maxLightCount];


void main() {
    gl_Position = MVP * vec4(vertex, 1.0);
    uv = texcoord;

    vec4 vertPos4 = MV * vec4(vertex, 1.0);
    vec3 vertPos = vec3(vertPos4) / vertPos4.w;
    vec3 norm = vec3(NM * vec4(normal, 0.0));

    vec3 color = material.ambient;
    for (int light = 0; light < lightCount; ++light) {
        vec3 lightDir = normalize(lights[light].position - vertPos);
        float lambertian = max(dot(lightDir, norm), 0.0);
        float specular = 0.0;
        if (lambertian > 0.0) {
            vec3 viewDir = normalize(-vertPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float specAngle = max(dot(reflectDir, viewDir), 0.0);
	    specular = specAngle * specAngle;
        }
        color += (material.diffuse  * lambertian +
           material.specular * specular) * lights[light].color;
    }
    fragColor = color;
}
);

const char* modelF = GLSL(
in vec2 uv;
in vec3 fragColor;

uniform sampler2D matTex;
layout (location = 0) out vec4 color;

void main() {
  color = vec4(fragColor, 1.0) * texture(matTex, uv);
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
    std::optional<Texture> texture;

    Material() : faceCount_(0) {
    }

    Material(Material&& rhs) {
        diffuse = rhs.diffuse;
        ambient = rhs.ambient;
        specular = rhs.specular;
        ibo_ = std::move(rhs.ibo_);
        if (rhs.texture) {
            texture = std::move(*rhs.texture);
        }
    }

    Material& operator=(Material&& rhs) {
        diffuse = rhs.diffuse;
        ambient = rhs.ambient;
        specular = rhs.specular;
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

    void bind(Program& program) const {
        ibo_.bind(Buffer::Target::ElementArray);
        if (texture) {
            texture->bind();
        }
        glUniform3fv(
            program.uniform("material.diffuse"), 1, glm::value_ptr(diffuse));
        glUniform3fv(
            program.uniform("material.ambient"), 1, glm::value_ptr(ambient));
        glUniform3fv(
            program.uniform("material.specular"), 1, glm::value_ptr(specular));
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
        buffers_(std::move(rhs.buffers_)), materials_(rhs.materials_) {
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

    void render() const {
        glBindVertexArray(id_);
        for (const auto& material : materials_) {
            Binder matBinder(
                material, std::reference_wrapper<Program>(*program_));
            glDrawElements(
                GL_TRIANGLES, material.faceCount(), GL_UNSIGNED_SHORT, nullptr);
        }
        glBindVertexArray(0);
    }
    static unsigned int programId() {
        return program_->getRawId();
    }

    static Program& program() {
        return *program_;
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
                source.visitChunks(len, [&source, &parentPath, &material](
                                            const M3dStream::Header& header) {
                    switch (header.id) {
                        case m3d::Chunk::TextureMap: {
                            auto filename =
                                source.readFilenameValue(header.len);
                            std::vector<uint8_t> buffer;
                            Asset asset((parentPath / filename).string());
                            if (asset.read(buffer) != Asset::Error) {
                                // TODO: texture cache
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

                        default:
                            return false;
                    }
                });
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
                            for (auto& v : mesh.texcoords) {
                                v.y = 1 - v.y;
                            }
                            return true;

                        default:
                            return false;
                    }
                });

                if (mesh.valid()) {
                    std::vector<Face> result;
                    size_t face = 0;
                    for (size_t mat = 0; mat != materialFaces.size(); ++mat) {
                        result.reserve(materialFaces[mat].size() * 3);
                        for (auto end = materialFaces[mat].size() + face;
                             face < end; ++face) {
                            result.push_back(mesh.faces[face].x);
                            result.push_back(mesh.faces[face].y);
                            result.push_back(mesh.faces[face].z);
                        }
                        materials_[mat].set(result);
                        result.clear();
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

    Impl(Impl&& rhs) noexcept :
        meshes_(std::move(rhs.meshes_)), materials_(std::move(rhs.materials_)) {
    }

    void render() const {
        for (const auto& mesh : meshes_) {
            mesh.render();
        }
    }

    bool valid() const {
        return !meshes_.empty();
    }
};

Model::Model(std::string_view filename, float scale) noexcept :
    pImpl_(filename, scale) {
}

Model::Model(Model&& rhs) noexcept : pImpl_(std::move(rhs.pImpl_)) {
}

Model::~Model() {
}

void Model::render(
    const glm::mat4& mvp, const glm::mat4& mv, const glm::mat4& nm) const {
    auto& program = Mesh::program();
    program.use();
    glUniform1i(program.uniform("matTex"), 0);
    glUniformMatrix4fv(program.uniform("MVP"), 1, false, glm::value_ptr(mvp));
    glUniformMatrix4fv(program.uniform("MV"), 1, false, glm::value_ptr(mv));
    glUniformMatrix4fv(program.uniform("NM"), 1, false, glm::value_ptr(nm));

    pImpl_->render();
}

bool Model::valid() const {
    return pImpl_->valid();
}

LightManager::Light::Light(const glm::vec3& pos, const glm::vec3& col) :
    position_(pos), color_(col) {
}

LightManager& LightManager::instance() {
    static LightManager self;
    auto program = Mesh::programId();
    for (auto index = 0u; index < maxLightCount; ++index) {
        auto prefix = std::string("lights[") + std::to_string(index) + "].";
        self.cache_[index].position =
            glGetUniformLocation(program, (prefix + "position").c_str());
        self.cache_[index].color =
            glGetUniformLocation(program, (prefix + "color").c_str());
    }
    return self;
}

void LightManager::push(const Light& light) {
    auto& program = Mesh::program();
    program.use();
    auto& self = instance();
    auto index = self.lights_.size();
    if (index + 1 > maxLightCount) {
        throw std::logic_error("Maximum lights exceeded");
    }
    self.lights_.push_back(light);
    glUniform1i(program.uniform("lightCount"), index + 1);
    glUniform3fv(
        self.cache_[index].position, 1, glm::value_ptr(light.position_));
    glUniform3fv(self.cache_[index].color, 1, glm::value_ptr(light.color_));
}

void LightManager::pop_back() {
    auto& lights = instance().lights_;
    lights.pop_back();
    auto& program = Mesh::program();
    program.use();
    glUniform1i(program.uniform("lightCount"), lights.size());
}

void LightManager::clear() {
    instance().lights_.clear();
    auto& program = Mesh::program();
    program.use();
    glUniform1i(program.uniform("lightCount"), 0);
}

void LightManager::update(unsigned index, const Light& light) {
    auto& program = Mesh::program();
    program.use();
    auto& self = instance();
    auto& current = self.lights_[index];
    if (current.position_ != light.position_) {
        current.position_ = light.position_;
        glUniform3fv(
            self.cache_[index].position, 1, glm::value_ptr(light.position_));
    }
    if (current.color_ != light.color_) {
        current.color_ = light.color_;
        glUniform3fv(self.cache_[index].color, 1, glm::value_ptr(light.color_));
    }
}

}  // namespace neat
