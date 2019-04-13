// Copyright 2018 Keri Oleg

#pragma once

#include <memory>
#include <array>
#include <vector>
#include <string_view>
#include <glm/mat4x4.hpp>

#include "NoCopy.hh"

namespace neat {

class LightManager : NoCopy {
    static const unsigned maxLightCount = 16;
    static LightManager& instance();

    struct UniformCache {
        unsigned int position;
        unsigned int color;
    };

  public:
    class Light {
        glm::vec3 position_;
        glm::vec3 color_;

        friend class LightManager;

      public:
        Light(const glm::vec3& pos, const glm::vec3& col);
    };

    static void push(const Light& light);
    static void pop_back();
    static void update(unsigned index, const Light& light);
    static void clear();

  private:
    std::vector<Light> lights_;
    std::array<UniformCache, maxLightCount> cache_;
};

class Model : private NoCopy {
    class Impl;
    std::unique_ptr<Impl> pImpl_;

  public:
    Model(std::string_view filename, float scale) noexcept;
    Model(Model&& rhs) noexcept;
    void render(
        const glm::mat4& mvp, const glm::mat4& mv, const glm::mat4& nm) const;
    bool valid() const;
    ~Model();
};

}  // namespace neat
