#include "async_dependency_tracker.h"
#include "threaded_task_runner.h"

namespace zylann {

AsyncDependencyTracker::AsyncDependencyTracker(int initial_count) : _count(initial_count), _aborted(false) {}

AsyncDependencyTracker::AsyncDependencyTracker(
		int initial_count, Span<IThreadedTask *> next_tasks, ScheduleNextTasksCallback scheduler_cb) :
		_count(initial_count), _aborted(false), _next_tasks_schedule_callback(scheduler_cb) {
	//
	CRASH_COND(scheduler_cb == nullptr);

	_next_tasks.resize(next_tasks.size());

	for (unsigned int i = 0; i < next_tasks.size(); ++i) {
		IThreadedTask *task = next_tasks[i];
#ifdef DEBUG_ENABLED
		for (unsigned int j = i + 1; j < next_tasks.size(); ++j) {
			// Cannot add twice the same task
			CRASH_COND(next_tasks[j] == task);
		}
#endif
		_next_tasks[i] = task;
	}
}

AsyncDependencyTracker::~AsyncDependencyTracker() {
	for (auto it = _next_tasks.begin(); it != _next_tasks.end(); ++it) {
		IThreadedTask *task = *it;
		memdelete(task);
	}
}

void AsyncDependencyTracker::post_complete() {
	ERR_FAIL_COND_MSG(_count <= 0, "post_complete() called more times than expected");
	ERR_FAIL_COND_MSG(_aborted == true, "post_complete() called after abortion");
	--_count;
	// Note, this class only allows decrementing this counter up to zero
	if (_count == 0) {
		_next_tasks_schedule_callback(to_span(_next_tasks));
		_next_tasks.clear();
	}
	// The idea of putting next tasks inside this class instead of the tasks directly,
	// is because it would require such tasks to do the job, but also because when waiting for multiple tasks,
	// which one has ownership is fuzzy. It could be any of them that finish last.
	// Putting next tasks in the tracker instead has a clear unique ownership.
}

} // namespace zylann
