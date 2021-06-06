#include "loader.hpp"

#include "fbx_loader.hpp"

static String get_file_path(const String &path) {
	String res;
	unsigned pos = path.rfind("/");

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

Base_loader *make_loader(const String &file_name) {
	return 0;
}

// CLASS Base_loader ===========================================================

Base_loader::Base_loader(const String &file_path) {
	update_paths(file_path);
}

//------------------------------------------------------------------------------

void Base_loader::update_paths(const String &file_path) {
	_file_path = file_path;
	_path = get_file_path(file_path);
}

// =============================================================================
} // namespace Loader
// =============================================================================
