#ifndef TEX_LOADER_HPP__
#define TEX_LOADER_HPP__

#include <string>

// =============================================================================
namespace Loader {
// =============================================================================

/**
  @namespace Tex_loader
  @brief Loading/writting openGL textures utilities (using Qt4 QImage class)

*/
// =============================================================================
namespace Tex_loader {
// =============================================================================

/// @param file_path : path to the texture image. We use Qt to parse the image
/// file, so what QImage can open this function can too.
/// @return An openGL textures
struct GlTex2D;
GlTex2D *load(const std::string &file_path);

} // namespace Tex_loader

} // namespace Loader

#endif // TEX_LOADER_HPP__
