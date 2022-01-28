#pragma once
#include "scene/3d/mtree/mesh/Mesh.hpp"

namespace MeshProcessing::Smoothing {
void smooth_mesh(Tree3DMesh &mesh, const int iterations, const float factor, std::vector<float> *weights = nullptr);
}
