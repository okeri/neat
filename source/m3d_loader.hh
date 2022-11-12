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

#include <cstdint>
#include <filesystem>
#include <Asset.hh>

#include <Model.hh>
#include <Log.hh>

#include "Material.hh"
#include "Mesh.hh"

namespace neat::m3d {

enum class Chunk : uint16_t {
    Invalid = 0,
    Main = 0x4d4d,
    Version = 0x0002,
    Float = 0x10,
    Byte = 0x11,
    ByteGamma = 0x12,
    FloatGamma = 0x13,
    PercentInt = 0x30,
    PercentFloat = 0x31,
    Editor = 0x3d3d,
    ObjectBlock = 0x4000,
    Mesh = 0x4100,
    Vertices = 0x4110,
    FacesDescr = 0x4120,
    FacesMaterial = 0x4130,
    SmoothGroup = 0x4150,
    MappingCoords = 0x4140,
    LocalCoordSystem = 0x4160,
    Light = 0x4600,
    SpotLight = 0x4610,
    Camera = 0x4700,
    MaterialBlock = 0xafff,
    MaterialName = 0xa000,
    AmbientColor = 0xa010,
    DiffuseColor = 0xa020,
    SpecularColor = 0xa030,
    SpecularExponent = 0xa040,
    Shininess = 0xa041,
    TextureMap = 0xa200,
    BumpMap = 0xa230,
    ReflectionMap = 0xa220,
    ReflectionMapFilename = 0xa300,
    ReflectionMapParam = 0xa351,
    KeyframerChunk = 0xb000,
    MeshInfoBlock = 0xb002,
    SpotLightBlock = 0xb007,
    Frames = 0xb008,
    ObjectName = 0xb010,
    ObjectPivotPoint = 0xb013,
    PositionTrack = 0xb020,
    RotationTrack = 0xb021,
    ScaleTrack = 0xb022,
    Hierarchy = 0xb030
};

class M3dStream : public Asset {
    uint32_t mainLength_ = 0;

  public:
#pragma pack(push, 1)
    struct Header {
        uint16_t id;
        uint32_t len;
    };
#pragma pack(pop)

    explicit M3dStream(std::string_view filename) noexcept : Asset(filename) {
        Header header;  // NOLINT(hicpp-member-init)
        if (Asset::valid() && read(&header, sizeof(Header)) != Error &&
            static_cast<Chunk>(header.id) == m3d::Chunk::Main) {
            mainLength_ = header.len;
        }
    }

    uint32_t length() const {
        return mainLength_;
    }

    template <class Callback>
    uint32_t visitChunks(uint32_t length, Callback callback) {
        Header header;  // NOLINT(hicpp-member-init)
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
            if (static_cast<Chunk>(header.id) ==
                m3d::Chunk::ReflectionMapFilename) {
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
            switch (static_cast<Chunk>(header.id)) {
                case m3d::Chunk::Float:
                case m3d::Chunk::FloatGamma:
                    read(&result, sizeof(glm::vec3));
                    return true;

                case m3d::Chunk::Byte:
                case m3d::Chunk::ByteGamma: {
                    glm::vec<3, uint8_t> byteVec;  // NOLINT(hicpp-member-init)
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
            switch (static_cast<Chunk>(header.id)) {
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

}  // namespace neat::m3d

namespace neat {

inline std::pair<std::vector<Mesh>, std::vector<Material>> loadModel(
    std::string_view filename) noexcept {
    auto parentPath = std::filesystem::path(filename).parent_path();
    m3d::M3dStream source(filename);
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<Mesh::Face> faces;
    std::pair<std::vector<Mesh>, std::vector<Material>> result;

    if (source.length() == 0) {
        Log() << "error: cannot open file " << filename;
        return result;
    }
    auto handleMaterialBlock = [&source, &parentPath, &result](uint32_t len) {
        Material material;
        source.visitChunks(len, [&source, &parentPath, &material](
                                    const m3d::M3dStream::Header& header) {
            switch (static_cast<m3d::Chunk>(header.id)) {
                case m3d::Chunk::TextureMap: {
                    auto filename = source.readFilenameValue(header.len);
                    std::vector<uint8_t> buffer;
                    Asset asset((parentPath / filename).string());
                    if (asset.read(buffer) != Asset::Error) {
                        material.setTexture(
                            Texture(Image(buffer.data(), buffer.size())));
                    } else {
                        Log() << "error: cannot load texture " << filename;
                    }
                }
                    return true;

                case m3d::Chunk::AmbientColor:
                    material.setAmbient(source.readVecValue(header.len));
                    return true;

                case m3d::Chunk::DiffuseColor:
                    material.setDiffuse(source.readVecValue(header.len));
                    return true;

                case m3d::Chunk::SpecularColor:
                    material.setSpecular(source.readVecValue(header.len));
                    return true;

                default:
                    return false;
            }
        });
        result.second.emplace_back(std::move(material));
        return true;
    };

    auto handleMeshBlock = [&](uint32_t len) {
        std::vector<uint16_t> materialFaces;
        std::vector<glm::vec<3, uint16_t>> faceDescriptos;
        auto handleFaces = [&source, &materialFaces](
                               const m3d::M3dStream::Header& header) {
            uint16_t count;
            switch (static_cast<m3d::Chunk>(header.id)) {
                case m3d::Chunk::FacesMaterial: {
                    source.readChars(header.len);
                    source.read(&count, sizeof(uint16_t));
                    materialFaces.resize(count);
                    source.read(materialFaces.data(),
                        sizeof(uint16_t) * count);  // here ?
                }
                    return true;

                case m3d::Chunk::SmoothGroup:
                    source.read(&count, sizeof(uint16_t));
                    source.seek(count * sizeof(uint32_t));
                    return true;

                default:
                    return false;
            }
        };

        source.visitChunks(len, [&](const m3d::M3dStream::Header& header) {
            uint16_t count;

            switch (static_cast<m3d::Chunk>(header.id)) {
                case m3d::Chunk::Vertices: {
                    source.read(&count, sizeof(uint16_t));
                    vertices.resize(count);
                    source.read(vertices.data(), sizeof(glm::vec3) * count);
                }
                    return true;

                case m3d::Chunk::FacesDescr: {
                    auto face = glm::vec<3, uint16_t>();
                    source.read(&count, sizeof(uint16_t));
                    faceDescriptos.reserve(count);
                    for (auto i = 0; i < count; ++i) {
                        source.read(&face, sizeof(face));
                        faceDescriptos.push_back(face);
                        source.seek(2);  // skip flags
                    }
                    source.visitChunks(
                        header.len - sizeof(uint16_t) -
                            (count * (sizeof(glm::vec<3, uint16_t>) + 2)),
                        handleFaces);
                }
                    return true;

                case m3d::Chunk::MappingCoords:
                    source.read(&count, sizeof(uint16_t));
                    texcoords.resize(count);
                    source.read(texcoords.data(), count * sizeof(glm::vec2));
                    for (auto& v : texcoords) {
                        v.y = 1 - v.y;
                    }
                    return true;

                default:
                    return false;
            }
        });

        if (!vertices.empty() && !faceDescriptos.empty()) {
            faces.reserve(materialFaces.size() * 3);
            for (auto face = 0u; face < materialFaces.size(); ++face) {
                faces.push_back(
                    static_cast<Mesh::Face>(faceDescriptos[face].x));
                faces.push_back(
                    static_cast<Mesh::Face>(faceDescriptos[face].y));
                faces.push_back(
                    static_cast<Mesh::Face>(faceDescriptos[face].z));
            }

            normals.resize(vertices.size(), glm::vec3(0, 0, 0));
            for (const auto& face : faceDescriptos) {
                auto faceNormal =
                    glm::cross(vertices[face.y] - vertices[face.x],
                        vertices[face.z] - vertices[face.x]);
                normals[face.x] += faceNormal;
                normals[face.y] += faceNormal;
                normals[face.z] += faceNormal;
            }

            for (auto& normal : normals) {
                normal = glm::normalize(normal);
            }
            result.first.emplace_back(vertices.data(), normals.data(),
                texcoords.data(), vertices.size(), faces.data(), faces.size(),
                result.second.size() - 1);
        }
        return true;
    };

    source.visitChunks(
        source.length(), [&source, handleMaterialBlock, handleMeshBlock](
                             const m3d::M3dStream::Header& header) {
            if (static_cast<m3d::Chunk>(header.id) == m3d::Chunk::Editor) {
                source.visitChunks(
                    header.len, [&source, handleMaterialBlock, handleMeshBlock](
                                    const m3d::M3dStream::Header& header) {
                        switch (static_cast<m3d::Chunk>(header.id)) {
                            case m3d::Chunk::ObjectBlock: {
                                auto read = source.readChars(header.len);
                                source.visitChunks(
                                    header.len - read.length() - 1,
                                    [handleMeshBlock](
                                        const m3d::M3dStream::Header& header) {
                                        if (static_cast<m3d::Chunk>(
                                                header.id) ==
                                            m3d::Chunk::Mesh) {
                                            return handleMeshBlock(header.len);
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
    return result;
}

}  // namespace neat
