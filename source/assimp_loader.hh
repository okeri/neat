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
#include <Model.hh>
#include <Log.hh>

#include "Mesh.hh"
#include "Material.hh"

namespace neat {

inline std::pair<std::vector<Mesh>, std::vector<Material>> loadModel(
    std::string_view filename) noexcept {
    Asset asset(filename);
    std::vector<uint8_t> data;
    std::pair<std::vector<Mesh>, std::vector<Material>> result;
    if (!asset.valid() || asset.read(data) == Asset::Error) {
        Log() << "error: cannot open file " << filename;
        return result;
    }

    static auto importer = Assimp::Importer();
    auto scene = const_cast<aiScene*>(
        importer.ReadFileFromMemory(data.data(), data.size(),
            aiProcess_GenSmoothNormals | aiProcess_SortByPType |
                aiProcess_Triangulate | aiProcess_FlipUVs));
    if (scene == nullptr || scene->mRootNode == nullptr) {
        Log() << importer.GetErrorString();
        return result;
    }
    auto parentPath = std::filesystem::path(filename).parent_path();
    result.second.resize(scene->mNumMaterials);

    aiColor3D color;
    for (auto i = 0u; i < scene->mNumMaterials; ++i) {
        auto& material = result.second[i];
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

    for (auto meshIndex = 0u; meshIndex < scene->mNumMeshes; ++meshIndex) {
        const auto* aimesh = scene->mMeshes[meshIndex];

        std::vector<glm::vec2> texcoords(aimesh->mNumVertices, {0., 0.});
        if (aimesh->HasTextureCoords(0)) {
            for (auto i = 0u; i < aimesh->mNumVertices; ++i) {
                texcoords[i].x = aimesh->mTextureCoords[0][i].x;
                texcoords[i].y = aimesh->mTextureCoords[0][i].y;
            }
        }

        std::vector<Mesh::Face> faces;
        faces.reserve(aimesh->mNumFaces * 3);

        for (auto i = 0u; i < aimesh->mNumFaces; ++i) {
            faces.push_back(aimesh->mFaces[i].mIndices[0]);
            faces.push_back(aimesh->mFaces[i].mIndices[1]);
            faces.push_back(aimesh->mFaces[i].mIndices[2]);
        }

        result.first.emplace_back(aimesh->mVertices, aimesh->mNormals,
            texcoords.data(), aimesh->mNumVertices, faces.data(), faces.size(),
            aimesh->mMaterialIndex);
    }
    return result;
}

}  // namespace neat
