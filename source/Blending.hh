#include <GLES3/gl3.h>
#include <NoCopy.hh>

// TODO: consider shader-based alpha-blending
class Blending : private neat::NoCopy {
  public:
    Blending() noexcept {
        glEnable(GL_BLEND);
    }
    ~Blending() noexcept {
        glDisable(GL_BLEND);
    }
};
