#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include "core/io/image.h"
#include "core/math/rect2.h"
#include "core/math/vector2.h"
#include "core/object/ref_counted.h"
#include "core/templates/local_vector.h"

class ImageUtils : public RefCounted {
	GDCLASS(ImageUtils, RefCounted)
public:
	static void _bind_methods();

	ImageUtils();
	~ImageUtils();

	void _init();

	Vector2 get_red_range(Ref<Image> image_ref, const Rect2 &rect) const;
	float get_red_sum(Ref<Image> image_ref, const Rect2 &rect) const;
	float get_red_sum_weighted(Ref<Image> image_ref, Ref<Image> brush_ref, const Vector2 &p_pos, float factor) const;
	void add_red_brush(Ref<Image> image_ref, Ref<Image> brush_ref, const Vector2 &p_pos, float factor) const;
	void lerp_channel_brush(Ref<Image> image_ref, Ref<Image> brush_ref, const Vector2 &p_pos, float factor, float target_value, int channel) const;
	void lerp_color_brush(Ref<Image> image_ref, Ref<Image> brush_ref, const Vector2 &p_pos, float factor, const Color &target_value) const;
	float generate_gaussian_brush(Ref<Image> image_ref) const;
	void blur_red_brush(Ref<Image> image_ref, Ref<Image> brush_ref, const Vector2 &p_pos, float factor);
	void paint_indexed_splat(Ref<Image> index_map_ref, Ref<Image> weight_map_ref, Ref<Image> brush_ref, const Vector2 &p_pos, int texture_index, float factor);
	//void erode_red_brush(Ref<Image> image_ref, Ref<Image> brush_ref, Vector2 p_pos, float factor);
	static Vector<float> get_heightmap_max_spacing(Ref<Image> index_map_ref, int tile_x_count, int tile_z_count);

private:
	LocalVector<float> _blur_buffer;
};

#endif // IMAGE_UTILS_H
