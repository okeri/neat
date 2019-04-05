#include <glm/gtc/type_ptr.hpp>
#include <Object.hh>

namespace neat {

Object::Object(const Model& model, const glm::mat4& vp, const glm::mat4& view,
    glm::mat4&& modelMatrix) noexcept :
    model_(model),
    vp_(vp),
    view_(view),
    mm_(modelMatrix) {
}

void Object::render() {
    model_.render(vp_ * mm_, view_ * mm_, glm::transpose(glm::inverse(mm_)));
}

void Object::translate(const glm::vec3& t) {
    mm_ = glm::translate(mm_, t);
}

void Object::rotate(float angle, const glm::vec3& r) {
    mm_ = glm::rotate(mm_, angle, r);
}

}  // namespace neat
