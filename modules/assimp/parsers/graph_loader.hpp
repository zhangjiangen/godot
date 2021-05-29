#ifndef GRAPH_LOADER_HPP__
#define GRAPH_LOADER_HPP__

#include "graph.hpp"
#include "loader.hpp"

// =============================================================================
namespace Loader {
// =============================================================================

class Graph_file : public Base_loader {
public:
	Graph_file(const std::string &file_name) :
			Base_loader(file_name) { import_file(file_name); }

	/// The loader type
	Loader_t type() const { return SKEL; }

	bool import_file(const std::string &file_path) {
		Base_loader::update_paths(file_path);
		_graph.clear();
		_graph.load_from_file(file_path.c_str());
		return true;
	}

	bool export_file(const std::string &file_path) {
		Base_loader::update_paths(file_path);
		_graph.save_to_file(file_path.c_str());
		return true;
	}

	/// @return parsed animations or NULL.
	void get_anims(std::vector<Base_anim_eval *> &anims) const { anims.clear(); }

	/// transform internal representation into generic representation
	/// which are the same here.
	void get_graph(Graph &graph) const { graph = _graph; }

private:
	Graph _graph;
};

} // namespace Loader

#endif // GRAPH_LOADER_HPP__
