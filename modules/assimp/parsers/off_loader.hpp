#ifndef OFF_LOADER_HPP__
#define OFF_LOADER_HPP__

#include "loader.hpp"
#include "loader_anims.hpp"
#include "loader_mesh.hpp"

/**
  @file off_loader.hpp
  @brief Holds data structure and utilities to store and parse an OFF file
*/
// =============================================================================
namespace Loader {
// =============================================================================

/// @brief Utility to parse a mesh in 'off' file format
class Off_file : public Base_loader {
public:
	Off_file(const std::string &file_name) :
			Base_loader(file_name) { import_file(file_name); }

	bool import_file(const std::string &file_path);
	/// Not implemented
	bool export_file(const std::string &file_path);

	/// Fill the scene 'tree' with the parsed OFF mesh
	/// if flag == 0 or EObj::test(flag, EObj::MESH)

	/// OFF files have no animation frame 'anims will be returned empty'
	void get_anims(std::vector<Base_anim_eval *> &anims) const { anims.clear(); }

	/// transform internal representation into generic representation
	/// which are the same here.
	void get_mesh(Abs_mesh &mesh) const {
		mesh.clear();
		mesh = _mesh;
	}

private:
	Abs_mesh _mesh;
};

} // namespace Loader

#endif //OFF_LOADER_HPP__
