#pragma once
#include "scene/3d/mtree/mesh/Mesh.hpp"

namespace Mtree::MeshProcessing::Smoothing {
void smooth_mesh(Mesh &mesh, const int iterations, const float factor, std::vector<float> *weights = nullptr);
}