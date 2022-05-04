/*************************************************************************/
/*  camera_matrix.h                                                      */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef CAMERA_MATRIX_H
#define CAMERA_MATRIX_H

#include "core/math/math_defs.h"
#include "core/math/rect2.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "core/math/vector4.h"
#include "core/templates/local_vector.h"

struct AABB;
struct Plane;
struct Rect2;
struct Transform3D;
struct Vector2;

struct CameraMatrix {
	enum Planes {
		PLANE_NEAR,
		PLANE_FAR,
		PLANE_LEFT,
		PLANE_TOP,
		PLANE_RIGHT,
		PLANE_BOTTOM
	};
	union {
		struct {
			Quaternion x;
			Quaternion y;
			Quaternion z;
			Quaternion w;
		};
		Quaternion q[4];
		real_t matrix[4][4];
	};

	CameraMatrix lerp(const CameraMatrix &p_to, const real_t &p_weight) const {
		CameraMatrix b;
		b.q[0] = q[0].slerp(p_to.q[0], p_weight);
		b.q[1] = q[1].slerp(p_to.q[1], p_weight);
		b.q[2] = q[2].slerp(p_to.q[2], p_weight);
		b.q[3] = q[3].slerp(p_to.q[3], p_weight);

		return b;
	}
	void operator=(const CameraMatrix &p_other) {
		q[0] = p_other.q[0];
		q[1] = p_other.q[1];
		q[2] = p_other.q[2];
		q[3] = p_other.q[3];
	}
	float determinant() const;
	void set_identity();
	void set_transform(const Transform3D &p_transform);
	void set_zero();
	void set_light_bias();
	void set_depth_correction(bool p_flip_y = true);
	void set_light_atlas_rect(const Rect2 &p_rect);
	void _set_perspective(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov) {
		set_perspective(p_fovy_degrees, p_aspect, p_z_near, p_z_far, p_flip_fov);
	}
	void set_perspective(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov = false);
	void set_perspective(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov, int p_eye, real_t p_intraocular_dist, real_t p_convergence_dist);
	void set_for_hmd(int p_eye, real_t p_aspect, real_t p_intraocular_dist, real_t p_display_width, real_t p_display_to_lens, real_t p_oversample, real_t p_z_near, real_t p_z_far);

	void _set_orthogonal(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_znear, real_t p_zfar) {
		set_orthogonal(p_left, p_right, p_bottom, p_top, p_znear, p_zfar);
	}
	void set_orthogonal(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_znear, real_t p_zfar);
	void set_orthogonal(real_t p_size, real_t p_aspect, real_t p_znear, real_t p_zfar, bool p_flip_fov = false);
	void set_frustum(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_near, real_t p_far);
	void set_frustum(real_t p_size, real_t p_aspect, Vector2 p_offset, real_t p_near, real_t p_far, bool p_flip_fov = false);
	void adjust_perspective_znear(real_t p_new_znear);

	static real_t get_fovy(real_t p_fovx, real_t p_aspect) {
		return Math::rad2deg(Math::atan(p_aspect * Math::tan(Math::deg2rad(p_fovx) * 0.5)) * 2.0);
	}

	real_t get_z_far() const;
	real_t get_z_near() const;
	real_t get_aspect() const;
	real_t get_fov() const;
	bool is_orthogonal() const;

	LocalVector<Plane> get_projection_planes(const Transform3D &p_transform) const;

	bool get_endpoints(const Transform3D &p_transform, Vector3 *p_8points) const;
	Vector2 get_viewport_half_extents() const;
	Vector2 get_far_plane_half_extents() const;
	Vector3 project_local_ray_normal(const Vector2 &p_pos) const;

	void invert();
	CameraMatrix inverse() const;

	CameraMatrix operator*(const CameraMatrix &p_matrix) const;
	CameraMatrix multiply(const CameraMatrix &p_matrix) const {
		return operator*(p_matrix);
	}

	Plane xform4(const Plane &p_vec4) const;
	_FORCE_INLINE_ Vector3 xform(const Vector3 &p_vec3) const;

	operator String() const;

	void scale_translate_to_fit(const AABB &p_aabb);
	void make_scale(const Vector3 &p_scale);
	int get_pixels_per_meter(int p_for_pixel_width) const;
	operator Transform3D() const;

	void flip_y();

	bool operator==(const CameraMatrix &p_cam) const {
		for (uint32_t i = 0; i < 4; i++) {
			for (uint32_t j = 0; j < 4; j++) {
				if (matrix[i][j] != p_cam.matrix[i][j]) {
					return false;
				}
			}
		}
		return true;
	}

	bool operator!=(const CameraMatrix &p_cam) const {
		return !(*this == p_cam);
	}

	float get_lod_multiplier() const;
	Vector4 get_gpu_mull_add() const;

	inline CameraMatrix(real_t xx, real_t xy, real_t xz, real_t xw, real_t yx, real_t yy, real_t yz, real_t yw, real_t zx, real_t zy, real_t zz, real_t zw, real_t wx, real_t wy, real_t wz, real_t ww) {
		matrix[0][0] = xx;
		matrix[0][1] = xy;
		matrix[0][2] = xz;
		matrix[0][3] = xw;
		matrix[1][0] = yx;
		matrix[1][1] = yy;
		matrix[1][2] = yz;
		matrix[1][3] = yw;
		matrix[2][0] = zx;
		matrix[2][1] = zy;
		matrix[2][2] = zz;
		matrix[2][3] = zw;
		matrix[3][0] = wx;
		matrix[3][1] = wy;
		matrix[3][2] = wz;
		matrix[3][3] = ww;
	}
	inline CameraMatrix(const CameraMatrix &p_other) {
		q[0] = p_other.q[0];
		q[1] = p_other.q[1];
		q[2] = p_other.q[2];
		q[3] = p_other.q[3];
	}
	CameraMatrix();
	CameraMatrix(const Transform3D &p_transform);
	~CameraMatrix();
};

Vector3 CameraMatrix::xform(const Vector3 &p_vec3) const {
	Vector3 ret;
	ret.x = matrix[0][0] * p_vec3.x + matrix[1][0] * p_vec3.y + matrix[2][0] * p_vec3.z + matrix[3][0];
	ret.y = matrix[0][1] * p_vec3.x + matrix[1][1] * p_vec3.y + matrix[2][1] * p_vec3.z + matrix[3][1];
	ret.z = matrix[0][2] * p_vec3.x + matrix[1][2] * p_vec3.y + matrix[2][2] * p_vec3.z + matrix[3][2];
	real_t w = matrix[0][3] * p_vec3.x + matrix[1][3] * p_vec3.y + matrix[2][3] * p_vec3.z + matrix[3][3];
	return ret / w;
}

#endif // CAMERA_MATRIX_H
