#include "loader_skel.hpp"

// =============================================================================
namespace Loader {
// =============================================================================

void compute_bone_lengths(Abs_skeleton &skel) {
	int nb_bones = skel._bones.size();
	for (int bone_id = 0; bone_id < nb_bones; bone_id++) {
		int nb_sons = skel._sons[bone_id].size();
		float mean_len = 0.f;
		for (int s = 0; s < nb_sons; s++) {
			int son_id = skel._sons[bone_id][s];

			Vector3 p0 = skel._bones[bone_id]._frame.origin;
			Vector3 p1 = skel._bones[son_id]._frame.origin;

			mean_len += (p0 - p1).length();
		}

		float len = nb_sons > 1 ? mean_len / (float)nb_sons : mean_len;
		skel._bones[bone_id]._length = len;
	}
}

} // namespace Loader
