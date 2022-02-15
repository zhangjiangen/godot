#ifndef INT_RANGE_2D_H
#define INT_RANGE_2D_H

#include "core/math/rect2.h"
#include "math_funcs.h"
#include "scene/3d/vector2i.h"

struct IntRange2D {
	int min_x;
	int min_y;
	int max_x;
	int max_y;

	static inline IntRange2D from_min_max(const Vector2 &min_pos, const Vector2 &max_pos) {
		return IntRange2D(Rect2(min_pos, max_pos));
	}

	static inline IntRange2D from_pos_size(const Vector2 &min_pos, const Vector2 &size) {
		return IntRange2D(Rect2(min_pos, size));
	}

	IntRange2D(const Rect2 &rect) {
		min_x = static_cast<int>(rect.position.x);
		min_y = static_cast<int>(rect.position.y);
		max_x = static_cast<int>(rect.position.x + rect.size.x);
		max_y = static_cast<int>(rect.position.y + rect.size.y);
	}

	inline bool is_inside(const Vector2i &size) const {
		return min_x >= size.x &&
				min_y >= size.y &&
				max_x <= size.x &&
				max_y <= size.y;
	}

	inline void clip(const Vector2i &size) {
		min_x = Math::clamp(min_x, 0, size.x);
		min_y = Math::clamp(min_y, 0, size.y);
		max_x = Math::clamp(max_x, 0, size.x);
		max_y = Math::clamp(max_y, 0, size.y);
	}

	inline void pad(int p) {
		min_x -= p;
		min_y -= p;
		max_x += p;
		max_y += p;
	}

	inline int get_width() const {
		return max_x - min_x;
	}

	inline int get_height() const {
		return max_y - min_y;
	}
};

#endif // INT_RANGE_2D_H
