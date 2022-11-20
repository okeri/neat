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

#include <array>
#include <Model.hh>

#ifdef ENABLE_ASSIMP
#include "assimp_loader.hh"
#else
#include "m3d_loader.hh"
#endif

namespace {

// clang-format off
const char* modelV = GLSL(

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;
layout (location = 3) in ivec4 boneids;
layout (location = 4) in vec4 weights;
layout (location = 5) in mat4 model;

const uint maxBones = 128u;


uniform mat4 view;
uniform mat4 vp;

out vec2 uv;
out vec4 vertPos;
out vec4 vertNorm;

void main() {
    mat4 mv = view * model;
    mat4 boneTrans = mat4(1.);
    vec4 pos = boneTrans * vec4(position, 1.0);
    vertPos = mv * pos;
    vertNorm = mv * (boneTrans * vec4(normal, 0.0));
    gl_Position = vp * model * pos;
    uv = texcoord;
}
);

const char* modelF = GLSL(

layout (location = 0) out vec4 color;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

const uint maxLightCount = 16u;

struct Sun {
    vec3 color;
    vec3 direction;
};

struct PointLight {
    vec3 color;
    vec3 position;
    float attenuation;
};

uniform sampler2D matTex;
uniform mat4 view;
uniform Material material;
uniform PointLight lights[maxLightCount];
uniform Sun sun;

in vec2 uv;
in vec4 vertPos;
in vec4 vertNorm;

vec3 calculateLight(vec4 direction, vec3 color) {
    vec4 lightDir = normalize(-direction);
    float Kd = dot(vertNorm, lightDir);
    vec3 diffuse = max(0., Kd) * color * material.diffuse;
    if (Kd < 0.) {
        return diffuse;
    }
    vec4 halfWayDir = lightDir + normalize(-vertPos);
    return diffuse + max(0., dot(vertNorm, halfWayDir)) * color * material.specular;
}

vec3 calculatePointLight(uint index) {
    vec4 direction = vertPos - (view * vec4(lights[index].position,1.0));
    float distance = length(direction);
    float att = 1. + (lights[index].attenuation * distance * distance);
    return calculateLight(direction, lights[index].color) / att;
}

void main() {
    vec3 vertColor = material.ambient;
    if (sun.color != vec3(0.)) {
        vertColor += calculateLight(vec4(sun.direction,1.), sun.color);
    }
    for (uint light = 0u; light < maxLightCount; ++light) {
        if (lights[light].color != vec3(0.)) {
            vertColor += calculatePointLight(light);
        } else {
     	    break;
	}
    }
    color = vec4(vertColor, 1.0) * texture(matTex, uv);
}
);

// clang-format on

}  // namespace

namespace neat {

Program& modelProgram() noexcept {
    static auto program =
        Program({{GL_FRAGMENT_SHADER, modelF}, {GL_VERTEX_SHADER, modelV}});
    return program;
}

class Model::Impl : private GLResource {
    enum Type : unsigned {
        Vertexes,
        Normals,
        TexCoords,
        Bones,
        Weights,
        Model,
        Count
    };

    class VAOBinder {
      public:
        explicit VAOBinder(unsigned id) noexcept {
            glBindVertexArray(id);
        }
        ~VAOBinder() noexcept {
            glBindVertexArray(0);
        }
    };

    std::array<Buffer, Type::Count> buffers_;
    std::vector<Mesh> meshes_;
    std::vector<Material> materials_;

  public:
    explicit Impl(ModelData&& data) noexcept :
        meshes_(std::move(data.meshes)), materials_(std::move(data.materials)) {
        buffers_[Model] = Buffer(Buffer::Target::Array, true);
        glGenVertexArrays(1, &id_);

        VAOBinder bind(id_);
        glEnableVertexAttribArray(Type::Vertexes);
        buffers_[Type::Vertexes].bind();
        buffers_[Type::Vertexes].set(data.vertices);
        glVertexAttribPointer(
            Type::Vertexes, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glEnableVertexAttribArray(Type::Normals);
        buffers_[Type::Normals].bind();
        buffers_[Type::Normals].set(data.normals);
        glVertexAttribPointer(Type::Normals, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glEnableVertexAttribArray(Type::TexCoords);
        buffers_[Type::TexCoords].bind();
        buffers_[Type::TexCoords].set(data.texcoords);
        glVertexAttribPointer(
            Type::TexCoords, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

        glEnableVertexAttribArray(Type::Bones);
        buffers_[Type::Bones].bind();
        glVertexAttribPointer(Type::Bones, 4, GL_INT, GL_FALSE, 0, nullptr);

        glEnableVertexAttribArray(Type::Weights);
        buffers_[Type::Weights].bind();
        glVertexAttribPointer(Type::Weights, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

        buffers_[Type::Model].bind();
        for (auto i = 0u; i < 4; ++i) {
            glEnableVertexAttribArray(Type::Model + i);
            glVertexAttribPointer(Type::Model + i, 4, GL_FLOAT, GL_FALSE,
                sizeof(glm::mat4),
                reinterpret_cast<const void*>(sizeof(glm::vec4) * i));
            glVertexAttribDivisor(Type::Model + i, 1);
        }
        glBindVertexArray(0);
    }

    Impl(Impl&& rhs) noexcept :
        GLResource(std::move(rhs)),
        buffers_(std::move(rhs.buffers_)),
        meshes_(std::move(rhs.meshes_)),
        materials_(std::move(rhs.materials_)) {
    }

    [[nodiscard]] bool valid() const noexcept {
        return !meshes_.empty() && !materials_.empty();
    }

    void setPos(const glm::mat4& pos) const noexcept {
        buffers_[Model].bind();
        buffers_[Model].set(glm::value_ptr(pos), sizeof(glm::mat4));
    }

    void setPos(const std::vector<glm::mat4>& pos) const noexcept {
        buffers_[Model].bind();
        buffers_[Model].set(pos);
    }

    void render(unsigned instances) const noexcept {
        VAOBinder bind(id_);
        auto& program = modelProgram();
        program.use();

        for (const auto& mesh : meshes_) {
            materials_[mesh.materialIndex()].bind(program);
            mesh.bind();
            mesh.render(instances);
        }
    }

    ~Impl() noexcept {
        glDeleteVertexArrays(1, &id_);
    }
};

Model::Model(std::string_view filename) noexcept : pImpl_(loadModel(filename)) {
}

Model::Model(Model&& rhs) noexcept : pImpl_(std::move(rhs.pImpl_)) {
}

Model::~Model() noexcept {
}

void Model::render(unsigned instances) const noexcept {
    pImpl_->render(instances);
}

bool Model::valid() const noexcept {
    return pImpl_->valid();
}

void Model::setPos(const glm::mat4& pos) const noexcept {
    pImpl_->setPos(pos);
}

void Model::setPos(const std::vector<glm::mat4>& pos) const noexcept {
    pImpl_->setPos(pos);
}

void Model::setLight(unsigned index, const glm::vec3& position,
    const glm::vec3& color, float attenuation) noexcept {
    auto& program = modelProgram();
    auto pos = std::string("lights[") + std::to_string(index) + "].";
    auto col = pos + "color";
    auto att = pos + "attenuation";
    pos += "position";
    program.use();
    glUniform3fv(program.uniform(pos.c_str()), 1, glm::value_ptr(position));
    glUniform3fv(program.uniform(col.c_str()), 1, glm::value_ptr(color));
    glUniform1f(program.uniform(att.c_str()), attenuation);
}

void Model::setSun(
    const glm::vec3& direction, const glm::vec3& color) noexcept {
    auto& program = modelProgram();
    program.use();
    glUniform3fv(
        program.uniform("sun.direction"), 1, glm::value_ptr(direction));
    glUniform3fv(program.uniform("sun.color"), 1, glm::value_ptr(color));
}

void Model::setVP(const glm::mat4& v, const glm::mat4& p) noexcept {
    auto& program = modelProgram();
    program.use();
    glUniformMatrix4fv(program.uniform("view"), 1, false, glm::value_ptr(v));
    glUniformMatrix4fv(program.uniform("vp"), 1, false, glm::value_ptr(p * v));
}

}  // namespace neat
