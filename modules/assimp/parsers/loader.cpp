#include "loader.hpp"

#include "fbx_loader.hpp"
#include "obj_loader.hpp"
#include "off_loader.hpp"

static std::string get_file_path(const std::string &path) {
	std::string res;
	unsigned pos = path.find_last_of('/');

	if ((pos + 1) == path.size())
		return path;
	else {
		if (pos < path.size())
			res = path.substr(0, pos + 1);
		else
			res = "";
	}

	return res;
}

// =============================================================================
namespace Loader {
// =============================================================================

Base_loader *make_loader(const std::string &file_name) {
	return 0;
}

// CLASS Base_loader ===========================================================

Base_loader::Base_loader(const std::string &file_path) {
	update_paths(file_path);
}

//------------------------------------------------------------------------------

void Base_loader::update_paths(const std::string &file_path) {
	_file_path = file_path;
	_path = get_file_path(file_path);
}

// =============================================================================
} // namespace Loader
// =============================================================================
