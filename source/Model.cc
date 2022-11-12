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

#ifdef ENABLE_ASSIMP
#include "assimp_loader.hh"
#else
#include "m3d_loader.hh"
#endif

namespace {

// clang-format off
const char* modelV = GLSL(

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;
//layout (location = 3) in u16vec3 faces;

struct Material {
    vec3 diffuse;
    vec3 ambient;
    vec3 specular;
};

const int maxLightCount = 16;

struct Light {
    vec3 position;
    vec3 color;
};

uniform mat4 MVP;
uniform mat4 MV;
uniform mat4 NM;
uniform Material material;
uniform Light lights[maxLightCount];

out vec2 uv;
out vec3 fragColor;

void main() {
    gl_Position = MVP * vec4(vertex, 1.0);
    uv = texcoord;

    vec4 vertPos4 = MV * vec4(vertex, 1.0);
    vec3 vertPos = vec3(vertPos4) / vertPos4.w;
    vec3 norm = vec3(NM * vec4(normal, 0.0));

    vec3 color = material.ambient;
    for (int light = 0; light < maxLightCount; ++light) {
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

}  // namespace

namespace neat {

class Model::Impl {
    std::vector<Mesh> meshes_;
    std::vector<Material> materials_;

  public:
    explicit Impl(
        std::pair<std::vector<Mesh>, std::vector<Material>>&& data) noexcept :
        meshes_(std::move(data.first)), materials_(std::move(data.second)) {
    }

    Impl(Impl&& rhs) noexcept :
        meshes_(std::move(rhs.meshes_)), materials_(std::move(rhs.materials_)) {
    }

    [[nodiscard]] bool valid() const noexcept {
        return !meshes_.empty() && !materials_.empty();
    }

    void render(Program& program) const noexcept {
        for (const auto& mesh : meshes_) {
            const auto& material = materials_[mesh.materialIndex()];
            mesh.bind();
            material.bind(program);
            mesh.render();
        }
        Mesh::unbind();
    }
};

Program& modelProgram() noexcept {
    static auto program =
        Program({{GL_FRAGMENT_SHADER, modelF}, {GL_VERTEX_SHADER, modelV}});
    return program;
}

Model::Model(std::string_view filename) noexcept : pImpl_(loadModel(filename)) {
}

Model::Model(Model&& rhs) noexcept : pImpl_(std::move(rhs.pImpl_)) {
}

Model::~Model() {
}

void Model::render(const glm::mat4& mvp, const glm::mat4& mv,
    const glm::mat4& nm) const noexcept {
    auto& program = modelProgram();
    program.use();
    glUniform1i(program.uniform("matTex"), 0);
    glUniformMatrix4fv(program.uniform("MVP"), 1, false, glm::value_ptr(mvp));
    glUniformMatrix4fv(program.uniform("MV"), 1, false, glm::value_ptr(mv));
    glUniformMatrix4fv(program.uniform("NM"), 1, false, glm::value_ptr(nm));
    pImpl_->render(program);
}

bool Model::valid() const noexcept {
    return pImpl_->valid();
}

void Model::setLight(unsigned index, const glm::vec3& position,
    const glm::vec3& color) noexcept {
    auto& program = modelProgram();
    auto pos = std::string("lights[") + std::to_string(index) + "].";
    auto col = pos + "color";
    pos += "position";
    program.use();
    glUniform3fv(program.uniform(pos.c_str()), 1, glm::value_ptr(position));
    glUniform3fv(program.uniform(col.c_str()), 1, glm::value_ptr(color));
}

}  // namespace neat
