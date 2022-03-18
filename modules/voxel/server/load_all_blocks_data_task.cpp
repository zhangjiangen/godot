#include "load_all_blocks_data_task.h"
#include "../util/macros.h"
#include "../util/profiling.h"
#include "voxel_server.h"

namespace zylann::voxel {

void LoadAllBlocksDataTask::run(zylann::ThreadedTaskContext ctx) {
	VOXEL_PROFILE_SCOPE();

	CRASH_COND(stream_dependency == nullptr);
	Ref<VoxelStream> stream = stream_dependency->stream;
	CRASH_COND(stream.is_null());

	stream->load_all_blocks(_result);

	//PRINT_VERBOSE(String("Loaded {0} blocks for volume {1}").format(varray(_result.blocks.size(), volume_id)));
}

int LoadAllBlocksDataTask::get_priority() {
	return 0;
}

bool LoadAllBlocksDataTask::is_cancelled() {
	return !stream_dependency->valid;
}

void LoadAllBlocksDataTask::apply_result() {
	if (VoxelServer::get_singleton()->is_volume_valid(volume_id)) {
		// TODO Comparing pointer may not be guaranteed
		// The request response must match the dependency it would have been requested with.
		// If it doesn't match, we are no longer interested in the result.
		if (stream_dependency->valid) {
			VoxelServer::VolumeCallbacks callbacks = VoxelServer::get_singleton()->get_volume_callbacks(volume_id);
			ERR_FAIL_COND(callbacks.data_output_callback == nullptr);

			for (auto it = _result.blocks.begin(); it != _result.blocks.end(); ++it) {
				VoxelStream::FullLoadingResult::Block &rb = *it;

				VoxelServer::BlockDataOutput o;
				o.voxels = rb.voxels;
				o.instances = std::move(rb.instances_data);
				o.position = rb.position;
				o.lod = rb.lod;
				o.dropped = false;
				o.max_lod_hint = false;
				o.initial_load = true;

				callbacks.data_output_callback(callbacks.data, o);
			}
		}

	} else {
		// This can happen if the user removes the volume while requests are still about to return
		//PRINT_VERBOSE("Stream data request response came back but volume wasn't found");
	}
}

} // namespace zylann::voxel