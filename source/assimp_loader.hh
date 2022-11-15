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

#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <Asset.hh>
#include <Log.hh>

#include "ModelData.hh"

namespace neat {

inline ModelData loadModel(std::string_view filename) noexcept {
    Asset asset(filename);
    std::vector<uint8_t> data;
    ModelData result;
    if (!asset.valid() || asset.read(data) == Asset::Error) {
        Log() << "error: cannot open file " << filename;
        return result;
    }

    static auto importer = Assimp::Importer();
    auto scene = const_cast<aiScene*>(
        importer.ReadFileFromMemory(data.data(), data.size(),
            aiProcess_GenNormals | aiProcess_FlipUVs |
                aiProcess_JoinIdenticalVertices));
    if (scene == nullptr || scene->mRootNode == nullptr) {
        Log() << importer.GetErrorString();
        return result;
    }
    auto parentPath = std::filesystem::path(filename).parent_path();
    result.materials.resize(scene->mNumMaterials);

    aiColor3D color;
    for (auto i = 0u; i < scene->mNumMaterials; ++i) {
        auto& material = result.materials[i];
        auto* aiMat = scene->mMaterials[i];
        if (aiMat->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
            material.setAmbient({color.r, color.g, color.b});
        }
        if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            material.setDiffuse({color.r, color.g, color.b});
        }
        if (aiMat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
            material.setSpecular({color.r, color.g, color.b});
        }

        auto getTexture = [&](aiTextureType texType) -> std::optional<Texture> {
            if (aiMat->GetTextureCount(texType) > 0) {
                aiString path;
                if (aiMat->GetTexture(texType, 0, &path, nullptr, nullptr,
                        nullptr, nullptr, nullptr) == AI_SUCCESS) {
                    if (const auto* aiTex =
                            scene->GetEmbeddedTexture(path.C_Str());
                        aiTex) {
                        // TODO: parse achFormatHint
                        Log() << "found embedded texture "
                              << aiTex->achFormatHint;
                        return Texture(
                            aiTex->pcData, 4, aiTex->mWidth, aiTex->mHeight);
                    } else {
                        std::vector<uint8_t> buffer;
                        Asset asset((parentPath / path.C_Str()).string());
                        if (asset.read(buffer) != Asset::Error) {
                            return Texture(Image(buffer.data(), buffer.size()));
                        }
                    }
                }
            }
            return std::nullopt;
        };
        // TODO: specular texture ? ambient texture ?
        auto tex = getTexture(aiTextureType_DIFFUSE);
        if (tex) {
            material.setTexture(std::move(tex.value()));
        }
    }
    auto vertCount = 0u;
    for (auto meshIndex = 0u; meshIndex < scene->mNumMeshes; ++meshIndex) {
        vertCount += scene->mMeshes[meshIndex]->mNumVertices;
    }
    result.vertices.reserve(vertCount);
    result.normals.reserve(vertCount);
    result.texcoords.resize(vertCount, {0.f, 0.f});

    for (auto meshIndex = 0u, offset = 0u; meshIndex < scene->mNumMeshes;
         ++meshIndex) {
        // TODO: partial buffer filling directly from assimp buffers to cpu
        // without memory copy
        const auto* aimesh = scene->mMeshes[meshIndex];
        result.vertices.insert(result.vertices.end(),
            reinterpret_cast<glm::vec3*>(aimesh->mVertices),
            reinterpret_cast<glm::vec3*>(aimesh->mVertices) +
                aimesh->mNumVertices);
        result.normals.insert(result.normals.end(),
            reinterpret_cast<glm::vec3*>(aimesh->mNormals),
            reinterpret_cast<glm::vec3*>(aimesh->mNormals) +
                aimesh->mNumVertices);
        if (aimesh->HasTextureCoords(0)) {
            for (auto i = 0u; i < aimesh->mNumVertices; ++i) {
                result.texcoords[i + offset].x = aimesh->mTextureCoords[0][i].x;
                result.texcoords[i + offset].y = aimesh->mTextureCoords[0][i].y;
            }
        }

        std::vector<Mesh::Face> faces;
        faces.reserve(aimesh->mNumFaces * 3);
        for (auto i = 0u; i < aimesh->mNumFaces; ++i) {
            const auto& face = aimesh->mFaces[i];
            faces.push_back(face.mIndices[0] + offset);
            faces.push_back(face.mIndices[1] + offset);
            faces.push_back(face.mIndices[2] + offset);
        }

        result.meshes.emplace_back(
            faces.data(), faces.size(), aimesh->mMaterialIndex);
        offset += aimesh->mNumVertices;
    }

    return result;
}

}  // namespace neat
