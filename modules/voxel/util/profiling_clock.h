#ifndef PROFILING_CLOCK_H
#define PROFILING_CLOCK_H

#include <core/os/time.h>

namespace zylann {

struct ProfilingClock {
	uint64_t time_before = 0;

	ProfilingClock() {
		restart();
	}

	uint64_t restart() {
		const uint64_t now = Time::get_singleton()->get_ticks_usec();
		const uint64_t time_spent = now - time_before;
		time_before = Time::get_singleton()->get_ticks_usec();
		return time_spent;
	}
};

} // namespace zylann

#endif // PROFILING_CLOCK_H
