# neat
simple low-level graphic engine

## Dependencies:
 * C++17 compatible compiler
 * [glm](https://glm.g-truc.net/)
 * [freetype2](https://www.freetype.org/)
 * [libpng](http://www.libpng.org/)
 * wayland-egl(Linux build only)
 * android-ndk(Android build only)

## TODO:
  * Texture cache (allow GPU texture sharing between various library classes)
  * Fong lightning model?
  * Basic visual effects (e.g. water, fog, fire, explosion?)
  * Extend text/layout support (all about UI, but i'll try to keep it simple)
  * Provide some files for building 
  + Simple sample app (including Java support code for android) 
  + add iOS support
  
## Done:
  * Render billboards
  * Render text using fonts
  * Load and render 3ds models
  * Support png textures
