/*************************************************************************/
/*  renderer_storage_rd.h                                                */
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

#ifndef RENDERING_SERVER_STORAGE_RD_H
#define RENDERING_SERVER_STORAGE_RD_H

#include "core/templates/list.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid_owner.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_rd/effects_rd.h"
#include "servers/rendering/renderer_rd/shaders/canvas_sdf.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/particles.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/particles_copy.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/skeleton.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/voxel_gi_sdf.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/canvas_texture_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_device.h"

#include "servers/rendering/shader_compiler.h"
class RendererStorageRD : public RendererStorage {
public:
	static _FORCE_INLINE_ void store_transform(const Transform3D &p_mtx, float *p_array) {
		p_array[0] = p_mtx.basis.elements[0][0];
		p_array[1] = p_mtx.basis.elements[1][0];
		p_array[2] = p_mtx.basis.elements[2][0];
		p_array[3] = 0;
		p_array[4] = p_mtx.basis.elements[0][1];
		p_array[5] = p_mtx.basis.elements[1][1];
		p_array[6] = p_mtx.basis.elements[2][1];
		p_array[7] = 0;
		p_array[8] = p_mtx.basis.elements[0][2];
		p_array[9] = p_mtx.basis.elements[1][2];
		p_array[10] = p_mtx.basis.elements[2][2];
		p_array[11] = 0;
		p_array[12] = p_mtx.origin.x;
		p_array[13] = p_mtx.origin.y;
		p_array[14] = p_mtx.origin.z;
		p_array[15] = 1;
	}

	static _FORCE_INLINE_ void store_basis_3x4(const Basis &p_mtx, float *p_array) {
		p_array[0] = p_mtx.elements[0][0];
		p_array[1] = p_mtx.elements[1][0];
		p_array[2] = p_mtx.elements[2][0];
		p_array[3] = 0;
		p_array[4] = p_mtx.elements[0][1];
		p_array[5] = p_mtx.elements[1][1];
		p_array[6] = p_mtx.elements[2][1];
		p_array[7] = 0;
		p_array[8] = p_mtx.elements[0][2];
		p_array[9] = p_mtx.elements[1][2];
		p_array[10] = p_mtx.elements[2][2];
		p_array[11] = 0;
	}

	static _FORCE_INLINE_ void store_transform_3x3(const Basis &p_mtx, float *p_array) {
		p_array[0] = p_mtx.elements[0][0];
		p_array[1] = p_mtx.elements[1][0];
		p_array[2] = p_mtx.elements[2][0];
		p_array[3] = 0;
		p_array[4] = p_mtx.elements[0][1];
		p_array[5] = p_mtx.elements[1][1];
		p_array[6] = p_mtx.elements[2][1];
		p_array[7] = 0;
		p_array[8] = p_mtx.elements[0][2];
		p_array[9] = p_mtx.elements[1][2];
		p_array[10] = p_mtx.elements[2][2];
		p_array[11] = 0;
	}

	static _FORCE_INLINE_ void store_transform_transposed_3x4(const Transform3D &p_mtx, float *p_array) {
		p_array[0] = p_mtx.basis.elements[0][0];
		p_array[1] = p_mtx.basis.elements[0][1];
		p_array[2] = p_mtx.basis.elements[0][2];
		p_array[3] = p_mtx.origin.x;
		p_array[4] = p_mtx.basis.elements[1][0];
		p_array[5] = p_mtx.basis.elements[1][1];
		p_array[6] = p_mtx.basis.elements[1][2];
		p_array[7] = p_mtx.origin.y;
		p_array[8] = p_mtx.basis.elements[2][0];
		p_array[9] = p_mtx.basis.elements[2][1];
		p_array[10] = p_mtx.basis.elements[2][2];
		p_array[11] = p_mtx.origin.z;
	}

	static _FORCE_INLINE_ void store_camera(const CameraMatrix &p_mtx, float *p_array) {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				p_array[i * 4 + j] = p_mtx.matrix[i][j];
			}
		}
	}

	static _FORCE_INLINE_ void store_soft_shadow_kernel(const float *p_kernel, float *p_array) {
		for (int i = 0; i < 128; i++) {
			p_array[i] = p_kernel[i];
		}
	}

	enum ShaderType {
		SHADER_TYPE_2D,
		SHADER_TYPE_3D,
		SHADER_TYPE_PARTICLES,
		SHADER_TYPE_SKY,
		SHADER_TYPE_FOG,
		SHADER_TYPE_MAX
	};

	struct ShaderData {
		virtual void set_code(const String &p_Code) = 0;
		virtual void set_default_texture_param(const StringName &p_name, RID p_texture, int p_index) = 0;
		virtual void get_param_list(List<PropertyInfo> *p_param_list) const = 0;

		virtual void get_instance_param_list(List<InstanceShaderParam> *p_param_list) const = 0;
		virtual bool is_param_texture(const StringName &p_param) const = 0;
		virtual bool is_animated() const = 0;
		virtual bool casts_shadows() const = 0;
		virtual Variant get_default_parameter(const StringName &p_parameter) const = 0;
		virtual RS::ShaderNativeSourceCode get_native_source_code() const { return RS::ShaderNativeSourceCode(); }

		virtual ~ShaderData() {}
	};

	typedef ShaderData *(*ShaderDataRequestFunction)();

	struct MaterialData {
		void update_uniform_buffer(const Map<StringName, ShaderLanguage::ShaderNode::Uniform> &p_uniforms, const uint32_t *p_uniform_offsets, const Map<StringName, Variant> &p_parameters, uint8_t *p_buffer, uint32_t p_buffer_size, bool p_use_linear_color);
		void update_textures(const Map<StringName, Variant> &p_parameters, const Map<StringName, Map<int, RID>> &p_default_textures, const Vector<ShaderCompiler::GeneratedCode::Texture> &p_texture_uniforms, RID *p_textures, bool p_use_linear_color);

		virtual void set_render_priority(int p_priority) = 0;
		virtual void set_next_pass(RID p_pass) = 0;
		virtual bool update_parameters(const Map<StringName, Variant> &p_parameters, bool p_uniform_dirty, bool p_textures_dirty) = 0;
		virtual ~MaterialData();

		//to be used internally by update_parameters, in the most common configuration of material parameters
		bool update_parameters_uniform_set(const Map<StringName, Variant> &p_parameters, bool p_uniform_dirty, bool p_textures_dirty, const Map<StringName, ShaderLanguage::ShaderNode::Uniform> &p_uniforms, const uint32_t *p_uniform_offsets, const Vector<ShaderCompiler::GeneratedCode::Texture> &p_texture_uniforms, const Map<StringName, Map<int, RID>> &p_default_texture_params, uint32_t p_ubo_size, RID &uniform_set, RID p_shader, uint32_t p_shader_uniform_set, uint32_t p_barrier = RD::BARRIER_MASK_ALL);
		void free_parameters_uniform_set(RID p_uniform_set);

	private:
		friend class RendererStorageRD;
		RID self;
		List<RID>::Element *global_buffer_E = nullptr;
		List<RID>::Element *global_texture_E = nullptr;
		uint64_t global_textures_pass = 0;
		Map<StringName, uint64_t> used_global_textures;

		//internally by update_parameters_uniform_set
		Vector<uint8_t> ubo_data;
		RID uniform_buffer;
		Vector<RID> texture_cache;
	};
	typedef MaterialData *(*MaterialDataRequestFunction)(ShaderData *);
	static void _material_uniform_set_erased(void *p_material);

	enum DefaultRDBuffer {
		DEFAULT_RD_BUFFER_VERTEX,
		DEFAULT_RD_BUFFER_NORMAL,
		DEFAULT_RD_BUFFER_TANGENT,
		DEFAULT_RD_BUFFER_COLOR,
		DEFAULT_RD_BUFFER_TEX_UV,
		DEFAULT_RD_BUFFER_TEX_UV2,
		DEFAULT_RD_BUFFER_CUSTOM0,
		DEFAULT_RD_BUFFER_CUSTOM1,
		DEFAULT_RD_BUFFER_CUSTOM2,
		DEFAULT_RD_BUFFER_CUSTOM3,
		DEFAULT_RD_BUFFER_BONES,
		DEFAULT_RD_BUFFER_WEIGHTS,
		DEFAULT_RD_BUFFER_MAX,
	};

private:
	/* TEXTURE API */

	RID default_rd_samplers[RS::CANVAS_ITEM_TEXTURE_FILTER_MAX][RS::CANVAS_ITEM_TEXTURE_REPEAT_MAX];
	RID custom_rd_samplers[RS::CANVAS_ITEM_TEXTURE_FILTER_MAX][RS::CANVAS_ITEM_TEXTURE_REPEAT_MAX];
	RID default_rd_storage_buffer;

	/* DECAL ATLAS */

	struct DecalAtlas {
		struct Texture {
			int panorama_to_dp_users;
			int users;
			Rect2 uv_rect;
		};

		struct SortItem {
			RID texture;
			Size2i pixel_size;
			Size2i size;
			Point2i pos;

			bool operator<(const SortItem &p_item) const {
				//sort larger to smaller
				if (size.height == p_item.size.height) {
					return size.width > p_item.size.width;
				} else {
					return size.height > p_item.size.height;
				}
			}
		};

		HashMap<RID, Texture> textures;
		bool dirty = true;
		int mipmaps = 5;

		RID texture;
		RID texture_srgb;
		struct MipMap {
			RID fb;
			RID texture;
			Size2i size;
		};
		Vector<MipMap> texture_mipmaps;

		Size2i size;
		DecalAtlas() {
			textures.set_debug_info(__FILE__, __LINE__);
		}

	} decal_atlas;

	void _update_decal_atlas();

	/* SHADER */

	struct Material;

	struct Shader {
		ShaderData *data;
		String code;
		ShaderType type;
		Map<StringName, Map<int, RID>> default_texture_parameter;
		Set<Material *> owners;
	};

	ShaderDataRequestFunction shader_data_request_func[SHADER_TYPE_MAX];
	mutable RID_Owner<Shader, true> shader_owner;

	/* Material */

	struct Material {
		RID self;
		MaterialData *data = nullptr;
		Shader *shader = nullptr;
		//shortcut to shader data and type
		ShaderType shader_type = SHADER_TYPE_MAX;
		uint32_t shader_id = 0;
		bool uniform_dirty = false;
		bool texture_dirty = false;
		Map<StringName, Variant> params;
		int32_t priority = 0;
		RID next_pass;
		SelfList<Material> update_element;

		Dependency dependency;

		Material() :
				update_element(this) {}
	};

	MaterialDataRequestFunction material_data_request_func[SHADER_TYPE_MAX];
	mutable RID_Owner<Material, true> material_owner;

	SelfList<Material>::List material_update_list;
	void _material_queue_update(Material *material, bool p_uniform, bool p_texture);
	void _update_queued_materials();

	/* Mesh */

	struct MeshInstance;

	struct Mesh {
		struct Surface {
			RS::PrimitiveType primitive = RS::PRIMITIVE_POINTS;
			uint32_t format = 0;

			RID vertex_buffer;
			RID attribute_buffer;
			RID skin_buffer;
			uint32_t vertex_count = 0;
			uint32_t vertex_buffer_size = 0;
			uint32_t skin_buffer_size = 0;

			// A different pipeline needs to be allocated
			// depending on the inputs available in the
			// material.
			// There are never that many geometry/material
			// combinations, so a simple array is the most
			// cache-efficient structure.

			struct Version {
				uint32_t input_mask = 0;
				RD::VertexFormatID vertex_format = 0;
				RID vertex_array;
			};

			SpinLock version_lock; //needed to access versions
			Version *versions = nullptr; //allocated on demand
			uint32_t version_count = 0;

			RID index_buffer;
			RID index_array;
			uint32_t index_count = 0;

			struct LOD {
				float edge_length = 0.0;
				uint32_t index_count = 0;
				RID index_buffer;
				RID index_array;
			};

			LOD *lods = nullptr;
			uint32_t lod_count = 0;

			AABB aabb;

			Vector<AABB> bone_aabbs;

			RID blend_shape_buffer;

			RID material;

			uint32_t render_index = 0;
			uint64_t render_pass = 0;

			uint32_t multimesh_render_index = 0;
			uint64_t multimesh_render_pass = 0;

			uint32_t particles_render_index = 0;
			uint64_t particles_render_pass = 0;

			RID uniform_set;
		};

		uint32_t blend_shape_count = 0;
		RS::BlendShapeMode blend_shape_mode = RS::BLEND_SHAPE_MODE_NORMALIZED;

		Surface **surfaces = nullptr;
		uint32_t surface_count = 0;

		Vector<AABB> bone_aabbs;

		bool has_bone_weights = false;

		AABB aabb;
		AABB custom_aabb;

		Vector<RID> material_cache;

		List<MeshInstance *> instances;

		RID shadow_mesh;
		Set<Mesh *> shadow_owners;

		Dependency dependency;
	};

	mutable RID_Owner<Mesh, true> mesh_owner;

	struct MeshInstance {
		Mesh *mesh;
		RID skeleton;
		struct Surface {
			RID vertex_buffer;
			RID uniform_set;

			Mesh::Surface::Version *versions = nullptr; //allocated on demand
			uint32_t version_count = 0;
		};
		LocalVector<Surface> surfaces;
		LocalVector<float> blend_weights;

		RID blend_weights_buffer;
		List<MeshInstance *>::Element *I = nullptr; //used to erase itself
		uint64_t skeleton_version = 0;
		bool dirty = false;
		bool weights_dirty = false;
		SelfList<MeshInstance> weight_update_list;
		SelfList<MeshInstance> array_update_list;
		MeshInstance() :
				weight_update_list(this), array_update_list(this) {}
	};

	void _mesh_instance_clear(MeshInstance *mi);
	void _mesh_instance_add_surface(MeshInstance *mi, Mesh *mesh, uint32_t p_surface);

	mutable RID_Owner<MeshInstance> mesh_instance_owner;

	SelfList<MeshInstance>::List dirty_mesh_instance_weights;
	SelfList<MeshInstance>::List dirty_mesh_instance_arrays;

	struct SkeletonShader {
		struct PushConstant {
			uint32_t has_normal;
			uint32_t has_tangent;
			uint32_t has_skeleton;
			uint32_t has_blend_shape;

			uint32_t vertex_count;
			uint32_t vertex_stride;
			uint32_t skin_stride;
			uint32_t skin_weight_offset;

			uint32_t blend_shape_count;
			uint32_t normalized_blend_shapes;
			uint32_t pad0;
			uint32_t pad1;
		};

		enum {
			UNIFORM_SET_INSTANCE = 0,
			UNIFORM_SET_SURFACE = 1,
			UNIFORM_SET_SKELETON = 2,
		};
		enum {
			SHADER_MODE_2D,
			SHADER_MODE_3D,
			SHADER_MODE_MAX
		};

		SkeletonShaderRD shader;
		RID version;
		RID version_shader[SHADER_MODE_MAX];
		RID pipeline[SHADER_MODE_MAX];

		RID default_skeleton_uniform_set;
	} skeleton_shader;

	void _mesh_surface_generate_version_for_input_mask(Mesh::Surface::Version &v, Mesh::Surface *s, uint32_t p_input_mask, MeshInstance::Surface *mis = nullptr);

	RID mesh_default_rd_buffers[DEFAULT_RD_BUFFER_MAX];

	/* MultiMesh */
	struct MultiMesh {
		RID mesh;
		int instances = 0;
		RS::MultimeshTransformFormat xform_format = RS::MULTIMESH_TRANSFORM_3D;
		bool uses_colors = false;
		bool uses_custom_data = false;
		int visible_instances = -1;
		AABB aabb;
		bool aabb_dirty = false;
		bool buffer_set = false;
		uint32_t stride_cache = 0;
		uint32_t color_offset_cache = 0;
		uint32_t custom_data_offset_cache = 0;

		Vector<float> data_cache; //used if individual setting is used
		bool *data_cache_dirty_regions = nullptr;
		uint32_t data_cache_used_dirty_regions = 0;

		RID buffer; //storage buffer
		RID uniform_set_3d;
		RID uniform_set_2d;

		bool dirty = false;
		MultiMesh *dirty_list = nullptr;

		Dependency dependency;
		Ref<MultMeshUserDataBase> UserDate;
	};

	mutable RID_Owner<MultiMesh, true> multimesh_owner;
	mutable Vector<RID> temp_mesh_list;

	MultiMesh *multimesh_dirty_list = nullptr;

	_FORCE_INLINE_ void _multimesh_make_local(MultiMesh *multimesh) const;
	_FORCE_INLINE_ void _multimesh_mark_dirty(MultiMesh *multimesh, int p_index, bool p_aabb);
	_FORCE_INLINE_ void _multimesh_mark_all_dirty(MultiMesh *multimesh, bool p_data, bool p_aabb);
	_FORCE_INLINE_ void _multimesh_re_create_aabb(MultiMesh *multimesh, const float *p_data, int p_instances);
	void _update_dirty_multimeshes();
	virtual void _multmesh_pre_render(const Transform3D &p_camera_transform, const CameraMatrix &p_camera_mat) override {
	}
	virtual void _multmesh_post_render() override {
	}

	/* PARTICLES */

	struct ParticleData {
		float xform[16];
		float velocity[3];
		uint32_t active;
		float color[4];
		float custom[3];
		float lifetime;
	};

	struct ParticlesFrameParams {
		enum {
			MAX_ATTRACTORS = 32,
			MAX_COLLIDERS = 32,
			MAX_3D_TEXTURES = 7
		};

		enum AttractorType {
			ATTRACTOR_TYPE_SPHERE,
			ATTRACTOR_TYPE_BOX,
			ATTRACTOR_TYPE_VECTOR_FIELD,
		};

		struct Attractor {
			float transform[16];
			float extents[3]; //exents or radius
			uint32_t type;

			uint32_t texture_index; //texture index for vector field
			float strength;
			float attenuation;
			float directionality;
		};

		enum CollisionType {
			COLLISION_TYPE_SPHERE,
			COLLISION_TYPE_BOX,
			COLLISION_TYPE_SDF,
			COLLISION_TYPE_HEIGHT_FIELD,
			COLLISION_TYPE_2D_SDF,

		};

		struct Collider {
			float transform[16];
			float extents[3]; //exents or radius
			uint32_t type;

			uint32_t texture_index; //texture index for vector field
			real_t scale;
			uint32_t pad[2];
		};

		uint32_t emitting;
		float system_phase;
		float prev_system_phase;
		uint32_t cycle;

		real_t explosiveness;
		real_t randomness;
		float time;
		float delta;

		uint32_t frame;
		uint32_t pad0;
		uint32_t pad1;
		uint32_t pad2;

		uint32_t random_seed;
		uint32_t attractor_count;
		uint32_t collider_count;
		float particle_size;

		float emission_transform[16];

		Attractor attractors[MAX_ATTRACTORS];
		Collider colliders[MAX_COLLIDERS];
	};

	struct ParticleEmissionBufferData {
	};

	struct ParticleEmissionBuffer {
		struct Data {
			float xform[16];
			float velocity[3];
			uint32_t flags;
			float color[4];
			float custom[4];
		};

		int32_t particle_count;
		int32_t particle_max;
		uint32_t pad1;
		uint32_t pad2;
		Data data[1]; //its 2020 and empty arrays are still non standard in C++
	};

	struct Particles {
		RS::ParticlesMode mode = RS::PARTICLES_MODE_3D;
		bool inactive = true;
		double inactive_time = 0.0;
		bool emitting = false;
		bool one_shot = false;
		int amount = 0;
		double lifetime = 1.0;
		double pre_process_time = 0.0;
		real_t explosiveness = 0.0;
		real_t randomness = 0.0;
		bool restart_request = false;
		AABB custom_aabb = AABB(Vector3(-4, -4, -4), Vector3(8, 8, 8));
		bool use_local_coords = true;
		bool has_collision_cache = false;

		bool has_sdf_collision = false;
		Transform2D sdf_collision_transform;
		Rect2 sdf_collision_to_screen;
		RID sdf_collision_texture;

		RID process_material;
		uint32_t frame_counter = 0;
		RS::ParticlesTransformAlign transform_align = RS::PARTICLES_TRANSFORM_ALIGN_DISABLED;

		RS::ParticlesDrawOrder draw_order = RS::PARTICLES_DRAW_ORDER_INDEX;

		Vector<RID> draw_passes;
		Vector<Transform3D> trail_bind_poses;
		bool trail_bind_poses_dirty = false;
		RID trail_bind_pose_buffer;
		RID trail_bind_pose_uniform_set;

		RID particle_buffer;
		RID particle_instance_buffer;
		RID frame_params_buffer;

		uint32_t userdata_count = 0;

		RID particles_material_uniform_set;
		RID particles_copy_uniform_set;
		RID particles_transforms_buffer_uniform_set;
		RID collision_textures_uniform_set;

		RID collision_3d_textures[ParticlesFrameParams::MAX_3D_TEXTURES];
		uint32_t collision_3d_textures_used = 0;
		RID collision_heightmap_texture;

		RID particles_sort_buffer;
		RID particles_sort_uniform_set;

		bool dirty = false;
		Particles *update_list = nullptr;

		RID sub_emitter;

		double phase = 0.0;
		double prev_phase = 0.0;
		uint64_t prev_ticks = 0;
		uint32_t random_seed = 0;

		uint32_t cycle_number = 0;

		double speed_scale = 1.0;

		int fixed_fps = 30;
		bool interpolate = true;
		bool fractional_delta = false;
		double frame_remainder = 0;
		real_t collision_base_size = 0.01;

		bool clear = true;

		bool force_sub_emit = false;

		Transform3D emission_transform;

		Vector<uint8_t> emission_buffer_data;

		ParticleEmissionBuffer *emission_buffer = nullptr;
		RID emission_storage_buffer;

		Set<RID> collisions;

		Dependency dependency;

		double trail_length = 1.0;
		bool trails_enabled = false;
		LocalVector<ParticlesFrameParams> frame_history;
		LocalVector<ParticlesFrameParams> trail_params;

		Particles() {
		}
	};

	void _particles_process(Particles *p_particles, double p_delta);
	void _particles_allocate_emission_buffer(Particles *particles);
	void _particles_free_data(Particles *particles);
	void _particles_update_buffers(Particles *particles);

	struct ParticlesShader {
		struct PushConstant {
			float lifetime;
			uint32_t clear;
			uint32_t total_particles;
			uint32_t trail_size;

			uint32_t use_fractional_delta;
			uint32_t sub_emitter_mode;
			uint32_t can_emit;
			uint32_t trail_pass;
		};

		ParticlesShaderRD shader;
		ShaderCompiler compiler;

		RID default_shader;
		RID default_material;
		RID default_shader_rd;

		RID base_uniform_set;

		struct CopyPushConstant {
			float sort_direction[3];
			uint32_t total_particles;

			uint32_t trail_size;
			uint32_t trail_total;
			float frame_delta;
			float frame_remainder;

			float align_up[3];
			uint32_t align_mode;

			uint32_t order_by_lifetime;
			uint32_t lifetime_split;
			uint32_t lifetime_reverse;
			uint32_t copy_mode_2d;

			float inv_emission_transform[16];
		};

		enum {
			MAX_USERDATAS = 6
		};
		enum {
			COPY_MODE_FILL_INSTANCES,
			COPY_MODE_FILL_SORT_BUFFER,
			COPY_MODE_FILL_INSTANCES_WITH_SORT_BUFFER,
			COPY_MODE_MAX,
		};

		ParticlesCopyShaderRD copy_shader;
		RID copy_shader_version;
		RID copy_pipelines[COPY_MODE_MAX * (MAX_USERDATAS + 1)];

		LocalVector<float> pose_update_buffer;

	} particles_shader;

	Particles *particle_update_list = nullptr;

	struct ParticlesShaderData : public ShaderData {
		bool valid;
		RID version;
		bool uses_collision = false;

		//PipelineCacheRD pipelines[SKY_VERSION_MAX];
		Map<StringName, ShaderLanguage::ShaderNode::Uniform> uniforms;
		Vector<ShaderCompiler::GeneratedCode::Texture> texture_uniforms;

		Vector<uint32_t> ubo_offsets;
		uint32_t ubo_size;

		String path;
		String code;
		Map<StringName, Map<int, RID>> default_texture_params;

		RID pipeline;

		bool uses_time = false;

		bool userdatas_used[ParticlesShader::MAX_USERDATAS] = {};
		uint32_t userdata_count = 0;

		virtual void set_code(const String &p_Code);
		virtual void set_default_texture_param(const StringName &p_name, RID p_texture, int p_index);
		virtual void get_param_list(List<PropertyInfo> *p_param_list) const;
		virtual void get_instance_param_list(List<RendererStorage::InstanceShaderParam> *p_param_list) const;
		virtual bool is_param_texture(const StringName &p_param) const;
		virtual bool is_animated() const;
		virtual bool casts_shadows() const;
		virtual Variant get_default_parameter(const StringName &p_parameter) const;
		virtual RS::ShaderNativeSourceCode get_native_source_code() const;

		ParticlesShaderData();
		virtual ~ParticlesShaderData();
	};

	ShaderData *_create_particles_shader_func();
	static RendererStorageRD::ShaderData *_create_particles_shader_funcs() {
		return base_singleton->_create_particles_shader_func();
	}

	struct ParticlesMaterialData : public MaterialData {
		ParticlesShaderData *shader_data = nullptr;
		RID uniform_set;

		virtual void set_render_priority(int p_priority) {}
		virtual void set_next_pass(RID p_pass) {}
		virtual bool update_parameters(const Map<StringName, Variant> &p_parameters, bool p_uniform_dirty, bool p_textures_dirty);
		virtual ~ParticlesMaterialData();
	};

	MaterialData *_create_particles_material_func(ParticlesShaderData *p_shader);
	static RendererStorageRD::MaterialData *_create_particles_material_funcs(ShaderData *p_shader) {
		return base_singleton->_create_particles_material_func(static_cast<ParticlesShaderData *>(p_shader));
	}

	void update_particles() override;

	mutable RID_Owner<Particles, true> particles_owner;

	/* Particles Collision */

	struct ParticlesCollision {
		RS::ParticlesCollisionType type = RS::PARTICLES_COLLISION_TYPE_SPHERE_ATTRACT;
		uint32_t cull_mask = 0xFFFFFFFF;
		float radius = 1.0;
		Vector3 extents = Vector3(1, 1, 1);
		float attractor_strength = 1.0;
		float attractor_attenuation = 1.0;
		float attractor_directionality = 0.0;
		RID field_texture;
		RID heightfield_texture;
		RID heightfield_fb;
		Size2i heightfield_fb_size;

		RS::ParticlesCollisionHeightfieldResolution heightfield_resolution = RS::PARTICLES_COLLISION_HEIGHTFIELD_RESOLUTION_1024;

		Dependency dependency;
	};

	mutable RID_Owner<ParticlesCollision, true> particles_collision_owner;

	struct ParticlesCollisionInstance {
		RID collision;
		Transform3D transform;
		bool active = false;
	};

	mutable RID_Owner<ParticlesCollisionInstance> particles_collision_instance_owner;

	/* FOG VOLUMES */

	struct FogVolume {
		RID material;
		Vector3 extents = Vector3(1, 1, 1);

		RS::FogVolumeShape shape = RS::FOG_VOLUME_SHAPE_BOX;

		Dependency dependency;
	};

	mutable RID_Owner<FogVolume, true> fog_volume_owner;

	/* visibility_notifier */

	struct VisibilityNotifier {
		AABB aabb;
		Callable enter_callback;
		Callable exit_callback;
		Dependency dependency;
	};

	mutable RID_Owner<VisibilityNotifier> visibility_notifier_owner;

	/* Skeleton */

	struct Skeleton {
		bool use_2d = false;
		int size = 0;
		Vector<float> data;
		RID buffer;

		bool dirty = false;
		Skeleton *dirty_list = nullptr;
		Transform2D base_transform_2d;

		RID uniform_set_3d;
		RID uniform_set_mi;

		uint64_t version = 1;

		Dependency dependency;
	};

	mutable RID_Owner<Skeleton, true> skeleton_owner;

	_FORCE_INLINE_ void _skeleton_make_dirty(Skeleton *skeleton);

	Skeleton *skeleton_dirty_list = nullptr;

	void _update_dirty_skeletons();

	/* LIGHT */

	struct Light {
		RS::LightType type;
		float param[RS::LIGHT_PARAM_MAX];
		Color color = Color(1, 1, 1, 1);
		RID projector;
		bool shadow = false;
		bool negative = false;
		bool reverse_cull = false;
		RS::LightBakeMode bake_mode = RS::LIGHT_BAKE_DYNAMIC;
		uint32_t max_sdfgi_cascade = 2;
		uint32_t cull_mask = 0xFFFFFFFF;
		bool distance_fade = false;
		real_t distance_fade_begin = 40.0;
		real_t distance_fade_shadow = 50.0;
		real_t distance_fade_length = 10.0;
		RS::LightOmniShadowMode omni_shadow_mode = RS::LIGHT_OMNI_SHADOW_DUAL_PARABOLOID;
		RS::LightDirectionalShadowMode directional_shadow_mode = RS::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL;
		bool directional_blend_splits = false;
		bool directional_sky_only = false;
		uint64_t version = 0;

		Dependency dependency;
	};

	mutable RID_Owner<Light, true> light_owner;

	/* REFLECTION PROBE */

	struct ReflectionProbe {
		RS::ReflectionProbeUpdateMode update_mode = RS::REFLECTION_PROBE_UPDATE_ONCE;
		int resolution = 256;
		float intensity = 1.0;
		RS::ReflectionProbeAmbientMode ambient_mode = RS::REFLECTION_PROBE_AMBIENT_ENVIRONMENT;
		Color ambient_color;
		float ambient_color_energy = 1.0;
		float max_distance = 0;
		Vector3 extents = Vector3(1, 1, 1);
		Vector3 origin_offset;
		bool interior = false;
		bool box_projection = false;
		bool enable_shadows = false;
		uint32_t cull_mask = (1 << 20) - 1;
		float mesh_lod_threshold = 0.01;

		Dependency dependency;
	};

	mutable RID_Owner<ReflectionProbe, true> reflection_probe_owner;

	/* DECAL */

	struct Decal {
		Vector3 extents = Vector3(1, 1, 1);
		RID textures[RS::DECAL_TEXTURE_MAX];
		float emission_energy = 1.0;
		float albedo_mix = 1.0;
		Color modulate = Color(1, 1, 1, 1);
		uint32_t cull_mask = (1 << 20) - 1;
		float upper_fade = 0.3;
		float lower_fade = 0.3;
		bool distance_fade = false;
		float distance_fade_begin = 10;
		float distance_fade_length = 1;
		float normal_fade = 0.0;

		Dependency dependency;
	};

	mutable RID_Owner<Decal, true> decal_owner;

	/* VOXEL GI */

	struct VoxelGI {
		RID octree_buffer;
		RID data_buffer;
		RID sdf_texture;

		uint32_t octree_buffer_size = 0;
		uint32_t data_buffer_size = 0;

		Vector<int> level_counts;

		int cell_count = 0;

		Transform3D to_cell_xform;
		AABB bounds;
		Vector3i octree_size;

		float dynamic_range = 2.0;
		float energy = 1.0;
		float bias = 1.4;
		float normal_bias = 0.0;
		float propagation = 0.7;
		bool interior = false;
		bool use_two_bounces = false;

		float anisotropy_strength = 0.5;

		uint32_t version = 1;
		uint32_t data_version = 1;

		Dependency dependency;
	};

	mutable RID_Owner<VoxelGI, true> voxel_gi_owner;

	/* REFLECTION PROBE */

	struct Lightmap {
		RID light_texture;
		bool uses_spherical_harmonics = false;
		bool interior = false;
		AABB bounds = AABB(Vector3(), Vector3(1, 1, 1));
		int32_t array_index = -1; //unassigned
		PackedVector3Array points;
		PackedColorArray point_sh;
		PackedInt32Array tetrahedra;
		PackedInt32Array bsp_tree;

		struct BSP {
			static const int32_t EMPTY_LEAF = INT32_MIN;
			float plane[4];
			int32_t over = EMPTY_LEAF, under = EMPTY_LEAF;
		};

		Dependency dependency;
	};

	bool using_lightmap_array; //high end uses this
	/* for high end */

	Vector<RID> lightmap_textures;

	uint64_t lightmap_array_version = 0;

	mutable RID_Owner<Lightmap, true> lightmap_owner;

	float lightmap_probe_capture_update_speed = 4;

	/* RENDER TARGET */

	struct RenderTarget {
		Size2i size;
		uint32_t view_count;
		RID framebuffer;
		RID color;

		//used for retrieving from CPU
		RD::DataFormat color_format = RD::DATA_FORMAT_R4G4_UNORM_PACK8;
		RD::DataFormat color_format_srgb = RD::DATA_FORMAT_R4G4_UNORM_PACK8;
		Image::Format image_format = Image::FORMAT_L8;

		bool flags[RENDER_TARGET_FLAG_MAX];

		bool sdf_enabled = false;

		RID backbuffer; //used for effects
		RID backbuffer_fb;
		RID backbuffer_mipmap0;

		Vector<RID> backbuffer_mipmaps;

		RID framebuffer_uniform_set;
		RID backbuffer_uniform_set;

		RID sdf_buffer_write;
		RID sdf_buffer_write_fb;
		RID sdf_buffer_process[2];
		RID sdf_buffer_read;
		RID sdf_buffer_process_uniform_sets[2];
		RS::ViewportSDFOversize sdf_oversize = RS::VIEWPORT_SDF_OVERSIZE_120_PERCENT;
		RS::ViewportSDFScale sdf_scale = RS::VIEWPORT_SDF_SCALE_50_PERCENT;
		Size2i process_size;

		//texture generated for this owner (nor RD).
		RID texture;
		bool was_used;

		//clear request
		bool clear_requested;
		Color clear_color;
	};

	mutable RID_Owner<RenderTarget> render_target_owner;

	void _clear_render_target(RenderTarget *rt);
	void _update_render_target(RenderTarget *rt);
	void _create_render_target_backbuffer(RenderTarget *rt);
	void _render_target_allocate_sdf(RenderTarget *rt);
	void _render_target_clear_sdf(RenderTarget *rt);
	Rect2i _render_target_get_sdf_rect(const RenderTarget *rt) const;

	struct RenderTargetSDF {
		enum {
			SHADER_LOAD,
			SHADER_LOAD_SHRINK,
			SHADER_PROCESS,
			SHADER_PROCESS_OPTIMIZED,
			SHADER_STORE,
			SHADER_STORE_SHRINK,
			SHADER_MAX
		};

		struct PushConstant {
			int32_t size[2];
			int32_t stride;
			int32_t shift;
			int32_t base_size[2];
			int32_t pad[2];
		};

		CanvasSdfShaderRD shader;
		RID shader_version;
		RID pipelines[SHADER_MAX];
	} rt_sdf;

	/* GLOBAL SHADER VARIABLES */

	struct GlobalVariables {
		enum {
			BUFFER_DIRTY_REGION_SIZE = 1024
		};
		struct Variable {
			Set<RID> texture_materials; // materials using this

			RS::GlobalVariableType type;
			Variant value;
			Variant override;
			int32_t buffer_index = 0; //for vectors
			int32_t buffer_elements = 0; //for vectors
		};

		HashMap<StringName, Variable> variables;

		struct Value {
			float x;
			float y;
			float z;
			float w;
		};

		struct ValueInt {
			int32_t x;
			int32_t y;
			int32_t z;
			int32_t w;
		};

		struct ValueUInt {
			uint32_t x;
			uint32_t y;
			uint32_t z;
			uint32_t w;
		};

		struct ValueUsage {
			uint32_t elements = 0;
		};

		List<RID> materials_using_buffer;
		List<RID> materials_using_texture;

		RID buffer;
		Value *buffer_values = nullptr;
		ValueUsage *buffer_usage = nullptr;
		bool *buffer_dirty_regions = nullptr;
		uint32_t buffer_dirty_region_count = 0;

		uint32_t buffer_size = 0;

		bool must_update_texture_materials = false;
		bool must_update_buffer_materials = false;

		HashMap<RID, int32_t> instance_buffer_pos;
		GlobalVariables() {
			variables.set_debug_info(__FILE__, __LINE__);
			instance_buffer_pos.set_debug_info(__FILE__, __LINE__);
		}

	} global_variables;

	int32_t _global_variable_allocate(uint32_t p_elements);
	void _global_variable_store_in_buffer(int32_t p_index, RS::GlobalVariableType p_type, const Variant &p_value);
	void _global_variable_mark_buffer_dirty(int32_t p_index, int32_t p_elements);

	void _update_global_variables();
	/* EFFECTS */

	EffectsRD *effects = nullptr;

public:
	RID decal_atlas_get_texture() const;
	RID decal_atlas_get_texture_srgb() const;
	_FORCE_INLINE_ Rect2 decal_atlas_get_texture_rect(RID p_texture) {
		DecalAtlas::Texture *t = decal_atlas.textures.getptr(p_texture);
		if (!t) {
			return Rect2();
		}

		return t->uv_rect;
	}

	//internal usage

	_FORCE_INLINE_ RID sampler_rd_get_default(RS::CanvasItemTextureFilter p_filter, RS::CanvasItemTextureRepeat p_repeat) {
		return default_rd_samplers[p_filter][p_repeat];
	}
	_FORCE_INLINE_ RID sampler_rd_get_custom(RS::CanvasItemTextureFilter p_filter, RS::CanvasItemTextureRepeat p_repeat) {
		return custom_rd_samplers[p_filter][p_repeat];
	}

	void sampler_rd_configure_custom(float mipmap_bias);

	void sampler_rd_set_default(float p_mipmap_bias);

	/* SHADER API */

	RID shader_allocate() override;
	void shader_initialize(RID p_shader) override;

	void shader_set_code(RID p_shader, const String &p_code) override;
	String shader_get_code(RID p_shader) const override;
	void shader_get_param_list(RID p_shader, List<PropertyInfo> *p_param_list) const override;

	void shader_set_default_texture_param(RID p_shader, const StringName &p_name, RID p_texture, int p_index) override;
	RID shader_get_default_texture_param(RID p_shader, const StringName &p_name, int p_index) const override;
	Variant shader_get_param_default(RID p_shader, const StringName &p_param) const override;
	void shader_set_data_request_function(ShaderType p_shader_type, ShaderDataRequestFunction p_function);

	virtual RS::ShaderNativeSourceCode shader_get_native_source_code(RID p_shader) const override;

	/* COMMON MATERIAL API */

	RID material_allocate() override;
	void material_initialize(RID p_material) override;

	void material_set_shader(RID p_material, RID p_shader) override;

	void material_set_param(RID p_material, const StringName &p_param, const Variant &p_value) override;
	Variant material_get_param(RID p_material, const StringName &p_param) const override;

	void material_set_next_pass(RID p_material, RID p_next_material) override;
	void material_set_render_priority(RID p_material, int priority) override;

	bool material_is_animated(RID p_material) override;
	bool material_casts_shadows(RID p_material) override;

	void material_get_instance_shader_parameters(RID p_material, List<InstanceShaderParam> *r_parameters) override;

	void material_update_dependency(RID p_material, DependencyTracker *p_instance) override;

	void material_set_data_request_function(ShaderType p_shader_type, MaterialDataRequestFunction p_function);

	_FORCE_INLINE_ uint32_t material_get_shader_id(RID p_material) {
		Material *material = material_owner.get_or_null(p_material);
		return material->shader_id;
	}

	_FORCE_INLINE_ MaterialData *material_get_data(RID p_material, ShaderType p_shader_type) {
		Material *material = material_owner.get_or_null(p_material);
		if (!material || material->shader_type != p_shader_type) {
			return nullptr;
		} else {
			return material->data;
		}
	}

	/* MESH API */

	RID mesh_allocate() override;
	void mesh_initialize(RID p_mesh) override;

	virtual void mesh_set_blend_shape_count(RID p_mesh, int p_blend_shape_count) override;

	/// Return stride
	virtual void mesh_add_surface(RID p_mesh, const RS::SurfaceData &p_surface) override;

	virtual int mesh_get_blend_shape_count(RID p_mesh) const override;

	virtual void mesh_set_blend_shape_mode(RID p_mesh, RS::BlendShapeMode p_mode) override;
	virtual RS::BlendShapeMode mesh_get_blend_shape_mode(RID p_mesh) const override;

	virtual void mesh_surface_update_vertex_region(RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t> &p_data) override;
	virtual void mesh_surface_update_attribute_region(RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t> &p_data) override;
	virtual void mesh_surface_update_skin_region(RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t> &p_data) override;

	virtual void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material) override;
	virtual RID mesh_surface_get_material(RID p_mesh, int p_surface) const override;

	virtual RS::SurfaceData mesh_get_surface(RID p_mesh, int p_surface) const override;

	virtual int mesh_get_surface_count(RID p_mesh) const override;

	virtual void mesh_set_custom_aabb(RID p_mesh, const AABB &p_aabb) override;
	virtual AABB mesh_get_custom_aabb(RID p_mesh) const override;

	virtual AABB mesh_get_aabb(RID p_mesh, RID p_skeleton = RID()) override;
	virtual void mesh_set_shadow_mesh(RID p_mesh, RID p_shadow_mesh) override;

	virtual void mesh_clear(RID p_mesh) override;

	virtual bool mesh_needs_instance(RID p_mesh, bool p_has_skeleton) override;

	/* MESH INSTANCE */

	virtual RID mesh_instance_create(RID p_base) override;
	virtual void mesh_instance_set_skeleton(RID p_mesh_instance, RID p_skeleton) override;
	virtual void mesh_instance_set_blend_shape_weight(RID p_mesh_instance, int p_shape, float p_weight) override;
	virtual void mesh_instance_check_for_update(RID p_mesh_instance) override;
	virtual void update_mesh_instances() override;

	_FORCE_INLINE_ const RID *mesh_get_surface_count_and_materials(RID p_mesh, uint32_t &r_surface_count) {
		Mesh *mesh = mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_COND_V(!mesh, nullptr);
		r_surface_count = mesh->surface_count;
		if (r_surface_count == 0) {
			return nullptr;
		}
		if (mesh->material_cache.is_empty()) {
			mesh->material_cache.resize(mesh->surface_count);
			for (uint32_t i = 0; i < r_surface_count; i++) {
				mesh->material_cache.write[i] = mesh->surfaces[i]->material;
			}
		}

		return mesh->material_cache.ptr();
	}

	_FORCE_INLINE_ void *mesh_get_surface(RID p_mesh, uint32_t p_surface_index) {
		Mesh *mesh = mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_COND_V(!mesh, nullptr);
		ERR_FAIL_UNSIGNED_INDEX_V(p_surface_index, mesh->surface_count, nullptr);

		return mesh->surfaces[p_surface_index];
	}

	_FORCE_INLINE_ RID mesh_get_shadow_mesh(RID p_mesh) {
		Mesh *mesh = mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_COND_V(!mesh, RID());

		return mesh->shadow_mesh;
	}

	_FORCE_INLINE_ RS::PrimitiveType mesh_surface_get_primitive(void *p_surface) {
		Mesh::Surface *surface = reinterpret_cast<Mesh::Surface *>(p_surface);
		return surface->primitive;
	}

	_FORCE_INLINE_ bool mesh_surface_has_lod(void *p_surface) const {
		Mesh::Surface *s = reinterpret_cast<Mesh::Surface *>(p_surface);
		return s->lod_count > 0;
	}

	_FORCE_INLINE_ uint32_t mesh_surface_get_vertices_drawn_count(void *p_surface) const {
		Mesh::Surface *s = reinterpret_cast<Mesh::Surface *>(p_surface);
		return s->index_count ? s->index_count : s->vertex_count;
	}

	_FORCE_INLINE_ uint32_t mesh_surface_get_lod(void *p_surface, float p_model_scale, float p_distance_threshold, float p_mesh_lod_threshold, uint32_t *r_index_count = nullptr) const {
		Mesh::Surface *s = reinterpret_cast<Mesh::Surface *>(p_surface);

		int32_t current_lod = -1;
		if (r_index_count) {
			*r_index_count = s->index_count;
		}
		for (uint32_t i = 0; i < s->lod_count; i++) {
			float screen_size = s->lods[i].edge_length * p_model_scale / p_distance_threshold;
			if (screen_size > p_mesh_lod_threshold) {
				break;
			}
			current_lod = i;
		}
		if (current_lod == -1) {
			return 0;
		} else {
			if (r_index_count) {
				*r_index_count = s->lods[current_lod].index_count;
			}
			return current_lod + 1;
		}
	}

	_FORCE_INLINE_ RID mesh_surface_get_index_array(void *p_surface, uint32_t p_lod) const {
		Mesh::Surface *s = reinterpret_cast<Mesh::Surface *>(p_surface);

		if (p_lod == 0) {
			return s->index_array;
		} else {
			return s->lods[p_lod - 1].index_array;
		}
	}

	_FORCE_INLINE_ void mesh_surface_get_vertex_arrays_and_format(void *p_surface, uint32_t p_input_mask, RID &r_vertex_array_rd, RD::VertexFormatID &r_vertex_format) {
		Mesh::Surface *s = reinterpret_cast<Mesh::Surface *>(p_surface);

		s->version_lock.lock();

		//there will never be more than, at much, 3 or 4 versions, so iterating is the fastest way

		for (uint32_t i = 0; i < s->version_count; i++) {
			if (s->versions[i].input_mask != p_input_mask) {
				continue;
			}
			//we have this version, hooray
			r_vertex_format = s->versions[i].vertex_format;
			r_vertex_array_rd = s->versions[i].vertex_array;
			s->version_lock.unlock();
			return;
		}

		uint32_t version = s->version_count;
		s->version_count++;
		s->versions = (Mesh::Surface::Version *)memrealloc(s->versions, sizeof(Mesh::Surface::Version) * s->version_count);

		_mesh_surface_generate_version_for_input_mask(s->versions[version], s, p_input_mask);

		r_vertex_format = s->versions[version].vertex_format;
		r_vertex_array_rd = s->versions[version].vertex_array;

		s->version_lock.unlock();
	}

	_FORCE_INLINE_ void mesh_instance_surface_get_vertex_arrays_and_format(RID p_mesh_instance, uint32_t p_surface_index, uint32_t p_input_mask, RID &r_vertex_array_rd, RD::VertexFormatID &r_vertex_format) {
		MeshInstance *mi = mesh_instance_owner.get_or_null(p_mesh_instance);
		ERR_FAIL_COND(!mi);
		Mesh *mesh = mi->mesh;
		ERR_FAIL_UNSIGNED_INDEX(p_surface_index, mesh->surface_count);

		MeshInstance::Surface *mis = &mi->surfaces[p_surface_index];
		Mesh::Surface *s = mesh->surfaces[p_surface_index];

		s->version_lock.lock();

		//there will never be more than, at much, 3 or 4 versions, so iterating is the fastest way

		for (uint32_t i = 0; i < mis->version_count; i++) {
			if (mis->versions[i].input_mask != p_input_mask) {
				continue;
			}
			//we have this version, hooray
			r_vertex_format = mis->versions[i].vertex_format;
			r_vertex_array_rd = mis->versions[i].vertex_array;
			s->version_lock.unlock();
			return;
		}

		uint32_t version = mis->version_count;
		mis->version_count++;
		mis->versions = (Mesh::Surface::Version *)memrealloc(mis->versions, sizeof(Mesh::Surface::Version) * mis->version_count);

		_mesh_surface_generate_version_for_input_mask(mis->versions[version], s, p_input_mask, mis);

		r_vertex_format = mis->versions[version].vertex_format;
		r_vertex_array_rd = mis->versions[version].vertex_array;

		s->version_lock.unlock();
	}

	_FORCE_INLINE_ RID mesh_get_default_rd_buffer(DefaultRDBuffer p_buffer) {
		ERR_FAIL_INDEX_V(p_buffer, DEFAULT_RD_BUFFER_MAX, RID());
		return mesh_default_rd_buffers[p_buffer];
	}

	_FORCE_INLINE_ uint32_t mesh_surface_get_render_pass_index(RID p_mesh, uint32_t p_surface_index, uint64_t p_render_pass, uint32_t *r_index) {
		Mesh *mesh = mesh_owner.get_or_null(p_mesh);
		Mesh::Surface *s = mesh->surfaces[p_surface_index];

		if (s->render_pass != p_render_pass) {
			(*r_index)++;
			s->render_pass = p_render_pass;
			s->render_index = *r_index;
		}

		return s->render_index;
	}

	_FORCE_INLINE_ uint32_t mesh_surface_get_multimesh_render_pass_index(RID p_mesh, uint32_t p_surface_index, uint64_t p_render_pass, uint32_t *r_index) {
		Mesh *mesh = mesh_owner.get_or_null(p_mesh);
		Mesh::Surface *s = mesh->surfaces[p_surface_index];

		if (s->multimesh_render_pass != p_render_pass) {
			(*r_index)++;
			s->multimesh_render_pass = p_render_pass;
			s->multimesh_render_index = *r_index;
		}

		return s->multimesh_render_index;
	}

	_FORCE_INLINE_ uint32_t mesh_surface_get_particles_render_pass_index(RID p_mesh, uint32_t p_surface_index, uint64_t p_render_pass, uint32_t *r_index) {
		Mesh *mesh = mesh_owner.get_or_null(p_mesh);
		Mesh::Surface *s = mesh->surfaces[p_surface_index];

		if (s->particles_render_pass != p_render_pass) {
			(*r_index)++;
			s->particles_render_pass = p_render_pass;
			s->particles_render_index = *r_index;
		}

		return s->particles_render_index;
	}

	/* MULTIMESH API */

	RID multimesh_allocate() override;
	void multimesh_initialize(RID p_multimesh) override;

	void multimesh_allocate_data(RID p_multimesh, int p_instances, RS::MultimeshTransformFormat p_transform_format, bool p_use_colors = false, bool p_use_custom_data = false) override;
	int multimesh_get_instance_count(RID p_multimesh) const override;
	void multimesh_set_user_date(RID p_multimesh, Ref<MultMeshUserDataBase> p_user_date) const;

	void multimesh_set_mesh(RID p_multimesh, RID p_mesh) override;
	void multimesh_instance_set_transform(RID p_multimesh, int p_index, const Transform3D &p_transform) override;
	void multimesh_instance_set_transform_2d(RID p_multimesh, int p_index, const Transform2D &p_transform) override;
	void multimesh_instance_set_color(RID p_multimesh, int p_index, const Color &p_color) override;
	void multimesh_instance_set_custom_data(RID p_multimesh, int p_index, const Color &p_color) override;

	RID multimesh_get_mesh(RID p_multimesh) const override;

	Transform3D multimesh_instance_get_transform(RID p_multimesh, int p_index) const override;
	Transform2D multimesh_instance_get_transform_2d(RID p_multimesh, int p_index) const override;
	Color multimesh_instance_get_color(RID p_multimesh, int p_index) const override;
	Color multimesh_instance_get_custom_data(RID p_multimesh, int p_index) const override;

	void multimesh_set_buffer(RID p_multimesh, const Vector<float> &p_buffer) override;
	Vector<float> multimesh_get_buffer(RID p_multimesh) const override;

	void multimesh_set_visible_instances(RID p_multimesh, int p_visible) override;
	int multimesh_get_visible_instances(RID p_multimesh) const override;

	AABB multimesh_get_aabb(RID p_multimesh) const override;

	_FORCE_INLINE_ RS::MultimeshTransformFormat multimesh_get_transform_format(RID p_multimesh) const {
		MultiMesh *multimesh = multimesh_owner.get_or_null(p_multimesh);
		return multimesh->xform_format;
	}

	_FORCE_INLINE_ bool multimesh_uses_colors(RID p_multimesh) const {
		MultiMesh *multimesh = multimesh_owner.get_or_null(p_multimesh);
		return multimesh->uses_colors;
	}

	_FORCE_INLINE_ bool multimesh_uses_custom_data(RID p_multimesh) const {
		MultiMesh *multimesh = multimesh_owner.get_or_null(p_multimesh);
		return multimesh->uses_custom_data;
	}

	_FORCE_INLINE_ uint32_t multimesh_get_instances_to_draw(RID p_multimesh) const {
		MultiMesh *multimesh = multimesh_owner.get_or_null(p_multimesh);
		if (multimesh->visible_instances >= 0) {
			return multimesh->visible_instances;
		}
		return multimesh->instances;
	}

	_FORCE_INLINE_ RID multimesh_get_3d_uniform_set(RID p_multimesh, RID p_shader, uint32_t p_set) const {
		MultiMesh *multimesh = multimesh_owner.get_or_null(p_multimesh);
		if (!multimesh->uniform_set_3d.is_valid()) {
			Vector<RD::Uniform> uniforms;
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 0;
			u.append_id(multimesh->buffer);
			uniforms.push_back(u);
			multimesh->uniform_set_3d = RD::get_singleton()->uniform_set_create(uniforms, p_shader, p_set);
		}

		return multimesh->uniform_set_3d;
	}

	_FORCE_INLINE_ RID multimesh_get_2d_uniform_set(RID p_multimesh, RID p_shader, uint32_t p_set) const {
		MultiMesh *multimesh = multimesh_owner.get_or_null(p_multimesh);
		if (!multimesh->uniform_set_2d.is_valid()) {
			Vector<RD::Uniform> uniforms;
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 0;
			u.append_id(multimesh->buffer);
			uniforms.push_back(u);
			multimesh->uniform_set_2d = RD::get_singleton()->uniform_set_create(uniforms, p_shader, p_set);
		}

		return multimesh->uniform_set_2d;
	}

	/* SKELETON API */

	RID skeleton_allocate() override;
	void skeleton_initialize(RID p_skeleton) override;

	void skeleton_allocate_data(RID p_skeleton, int p_bones, bool p_2d_skeleton = false) override;
	void skeleton_set_base_transform_2d(RID p_skeleton, const Transform2D &p_base_transform) override;
	void skeleton_set_world_transform(RID p_skeleton, bool p_enable, const Transform3D &p_world_transform);
	int skeleton_get_bone_count(RID p_skeleton) const override;
	void skeleton_bone_set_transform(RID p_skeleton, int p_bone, const Transform3D &p_transform) override;
	Transform3D skeleton_bone_get_transform(RID p_skeleton, int p_bone) const override;
	void skeleton_bone_set_transform_2d(RID p_skeleton, int p_bone, const Transform2D &p_transform) override;
	Transform2D skeleton_bone_get_transform_2d(RID p_skeleton, int p_bone) const override;

	_FORCE_INLINE_ bool skeleton_is_valid(RID p_skeleton) {
		return skeleton_owner.get_or_null(p_skeleton) != nullptr;
	}

	_FORCE_INLINE_ RID skeleton_get_3d_uniform_set(RID p_skeleton, RID p_shader, uint32_t p_set) const {
		Skeleton *skeleton = skeleton_owner.get_or_null(p_skeleton);
		ERR_FAIL_COND_V(!skeleton, RID());
		ERR_FAIL_COND_V(skeleton->size == 0, RID());
		if (skeleton->use_2d) {
			return RID();
		}
		if (!skeleton->uniform_set_3d.is_valid()) {
			Vector<RD::Uniform> uniforms;
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 0;
			u.append_id(skeleton->buffer);
			uniforms.push_back(u);
			skeleton->uniform_set_3d = RD::get_singleton()->uniform_set_create(uniforms, p_shader, p_set);
		}

		return skeleton->uniform_set_3d;
	}
	/* Light API */

	void _light_initialize(RID p_rid, RS::LightType p_type);

	RID directional_light_allocate() override;
	void directional_light_initialize(RID p_light) override;

	RID omni_light_allocate() override;
	void omni_light_initialize(RID p_light) override;

	RID spot_light_allocate() override;
	void spot_light_initialize(RID p_light) override;

	void light_set_color(RID p_light, const Color &p_color);
	void light_set_param(RID p_light, RS::LightParam p_param, float p_value);
	void light_set_shadow(RID p_light, bool p_enabled);
	void light_set_projector(RID p_light, RID p_texture);
	void light_set_negative(RID p_light, bool p_enable);
	void light_set_cull_mask(RID p_light, uint32_t p_mask);
	void light_set_distance_fade(RID p_light, bool p_enabled, float p_begin, float p_shadow, float p_length);
	void light_set_reverse_cull_face_mode(RID p_light, bool p_enabled);
	void light_set_bake_mode(RID p_light, RS::LightBakeMode p_bake_mode);
	void light_set_max_sdfgi_cascade(RID p_light, uint32_t p_cascade);

	void light_omni_set_shadow_mode(RID p_light, RS::LightOmniShadowMode p_mode) override;

	void light_directional_set_shadow_mode(RID p_light, RS::LightDirectionalShadowMode p_mode) override;
	void light_directional_set_blend_splits(RID p_light, bool p_enable) override;
	bool light_directional_get_blend_splits(RID p_light) const override;
	void light_directional_set_sky_only(RID p_light, bool p_sky_only) override;
	bool light_directional_is_sky_only(RID p_light) const override;

	RS::LightDirectionalShadowMode light_directional_get_shadow_mode(RID p_light) override;
	RS::LightOmniShadowMode light_omni_get_shadow_mode(RID p_light) override;

	_FORCE_INLINE_ RS::LightType light_get_type(RID p_light) const override {
		const Light *light = light_owner.get_or_null(p_light);
		ERR_FAIL_COND_V(!light, RS::LIGHT_DIRECTIONAL);

		return light->type;
	}
	AABB light_get_aabb(RID p_light) const override;

	_FORCE_INLINE_ float light_get_param(RID p_light, RS::LightParam p_param) override {
		const Light *light = light_owner.get_or_null(p_light);
		ERR_FAIL_COND_V(!light, 0);

		return light->param[p_param];
	}

	_FORCE_INLINE_ RID light_get_projector(RID p_light) {
		const Light *light = light_owner.get_or_null(p_light);
		ERR_FAIL_COND_V(!light, RID());

		return light->projector;
	}

	_FORCE_INLINE_ Color light_get_color(RID p_light) override {
		const Light *light = light_owner.get_or_null(p_light);
		ERR_FAIL_COND_V(!light, Color());

		return light->color;
	}

	_FORCE_INLINE_ uint32_t light_get_cull_mask(RID p_light) {
		const Light *light = light_owner.get_or_null(p_light);
		ERR_FAIL_COND_V(!light, 0);

		return light->cull_mask;
	}

	_FORCE_INLINE_ bool light_is_distance_fade_enabled(RID p_light) {
		const Light *light = light_owner.get_or_null(p_light);
		return light->distance_fade;
	}

	_FORCE_INLINE_ float light_get_distance_fade_begin(RID p_light) {
		const Light *light = light_owner.get_or_null(p_light);
		return light->distance_fade_begin;
	}

	_FORCE_INLINE_ float light_get_distance_fade_shadow(RID p_light) {
		const Light *light = light_owner.get_or_null(p_light);
		return light->distance_fade_shadow;
	}

	_FORCE_INLINE_ float light_get_distance_fade_length(RID p_light) {
		const Light *light = light_owner.get_or_null(p_light);
		return light->distance_fade_length;
	}

	_FORCE_INLINE_ bool light_has_shadow(RID p_light) const {
		const Light *light = light_owner.get_or_null(p_light);
		ERR_FAIL_COND_V(!light, RS::LIGHT_DIRECTIONAL);

		return light->shadow;
	}

	_FORCE_INLINE_ bool light_has_projector(RID p_light) const override {
		const Light *light = light_owner.get_or_null(p_light);
		ERR_FAIL_COND_V(!light, RS::LIGHT_DIRECTIONAL);

		return light_owner.owns(light->projector);
	}

	_FORCE_INLINE_ bool light_is_negative(RID p_light) const {
		const Light *light = light_owner.get_or_null(p_light);
		ERR_FAIL_COND_V(!light, RS::LIGHT_DIRECTIONAL);

		return light->negative;
	}

	_FORCE_INLINE_ float light_get_transmittance_bias(RID p_light) const {
		const Light *light = light_owner.get_or_null(p_light);
		ERR_FAIL_COND_V(!light, 0.0);

		return light->param[RS::LIGHT_PARAM_TRANSMITTANCE_BIAS];
	}

	_FORCE_INLINE_ float light_get_shadow_volumetric_fog_fade(RID p_light) const {
		const Light *light = light_owner.get_or_null(p_light);
		ERR_FAIL_COND_V(!light, 0.0);

		return light->param[RS::LIGHT_PARAM_SHADOW_VOLUMETRIC_FOG_FADE];
	}

	RS::LightBakeMode light_get_bake_mode(RID p_light) override;
	uint32_t light_get_max_sdfgi_cascade(RID p_light) override;
	uint64_t light_get_version(RID p_light) const override;

	/* PROBE API */

	RID reflection_probe_allocate() override;
	void reflection_probe_initialize(RID p_reflection_probe) override;

	void reflection_probe_set_update_mode(RID p_probe, RS::ReflectionProbeUpdateMode p_mode) override;
	void reflection_probe_set_intensity(RID p_probe, float p_intensity) override;
	void reflection_probe_set_ambient_mode(RID p_probe, RS::ReflectionProbeAmbientMode p_mode) override;
	void reflection_probe_set_ambient_color(RID p_probe, const Color &p_color) override;
	void reflection_probe_set_ambient_energy(RID p_probe, float p_energy) override;
	void reflection_probe_set_max_distance(RID p_probe, float p_distance) override;
	void reflection_probe_set_extents(RID p_probe, const Vector3 &p_extents) override;
	void reflection_probe_set_origin_offset(RID p_probe, const Vector3 &p_offset) override;
	void reflection_probe_set_as_interior(RID p_probe, bool p_enable) override;
	void reflection_probe_set_enable_box_projection(RID p_probe, bool p_enable) override;
	void reflection_probe_set_enable_shadows(RID p_probe, bool p_enable) override;
	void reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers) override;
	void reflection_probe_set_resolution(RID p_probe, int p_resolution) override;
	void reflection_probe_set_mesh_lod_threshold(RID p_probe, float p_ratio) override;

	AABB reflection_probe_get_aabb(RID p_probe) const override;
	RS::ReflectionProbeUpdateMode reflection_probe_get_update_mode(RID p_probe) const override;
	uint32_t reflection_probe_get_cull_mask(RID p_probe) const override;
	Vector3 reflection_probe_get_extents(RID p_probe) const override;
	Vector3 reflection_probe_get_origin_offset(RID p_probe) const override;
	float reflection_probe_get_origin_max_distance(RID p_probe) const override;
	float reflection_probe_get_mesh_lod_threshold(RID p_probe) const override;

	int reflection_probe_get_resolution(RID p_probe) const;
	bool reflection_probe_renders_shadows(RID p_probe) const override;

	float reflection_probe_get_intensity(RID p_probe) const;
	bool reflection_probe_is_interior(RID p_probe) const;
	bool reflection_probe_is_box_projection(RID p_probe) const;
	RS::ReflectionProbeAmbientMode reflection_probe_get_ambient_mode(RID p_probe) const;
	Color reflection_probe_get_ambient_color(RID p_probe) const;
	float reflection_probe_get_ambient_color_energy(RID p_probe) const;

	void base_update_dependency(RID p_base, DependencyTracker *p_instance) override;
	void skeleton_update_dependency(RID p_skeleton, DependencyTracker *p_instance) override;

	/* DECAL API */

	RID decal_allocate() override;
	void decal_initialize(RID p_decal) override;

	virtual void decal_set_extents(RID p_decal, const Vector3 &p_extents) override;
	virtual void decal_set_texture(RID p_decal, RS::DecalTexture p_type, RID p_texture) override;
	virtual void decal_set_emission_energy(RID p_decal, float p_energy) override;
	virtual void decal_set_albedo_mix(RID p_decal, float p_mix) override;
	virtual void decal_set_modulate(RID p_decal, const Color &p_modulate) override;
	virtual void decal_set_cull_mask(RID p_decal, uint32_t p_layers) override;
	virtual void decal_set_distance_fade(RID p_decal, bool p_enabled, float p_begin, float p_length) override;
	virtual void decal_set_fade(RID p_decal, float p_above, float p_below) override;
	virtual void decal_set_normal_fade(RID p_decal, float p_fade) override;

	void decal_atlas_mark_dirty_on_texture(RID p_texture);
	void decal_atlas_remove_texture(RID p_texture);

	virtual void texture_add_to_decal_atlas(RID p_texture, bool p_panorama_to_dp = false);
	virtual void texture_remove_from_decal_atlas(RID p_texture, bool p_panorama_to_dp = false);

	_FORCE_INLINE_ Vector3 decal_get_extents(RID p_decal) {
		const Decal *decal = decal_owner.get_or_null(p_decal);
		return decal->extents;
	}

	_FORCE_INLINE_ RID decal_get_texture(RID p_decal, RS::DecalTexture p_texture) {
		const Decal *decal = decal_owner.get_or_null(p_decal);
		return decal->textures[p_texture];
	}

	_FORCE_INLINE_ Color decal_get_modulate(RID p_decal) {
		const Decal *decal = decal_owner.get_or_null(p_decal);
		return decal->modulate;
	}

	_FORCE_INLINE_ float decal_get_emission_energy(RID p_decal) {
		const Decal *decal = decal_owner.get_or_null(p_decal);
		return decal->emission_energy;
	}

	_FORCE_INLINE_ float decal_get_albedo_mix(RID p_decal) {
		const Decal *decal = decal_owner.get_or_null(p_decal);
		return decal->albedo_mix;
	}

	_FORCE_INLINE_ uint32_t decal_get_cull_mask(RID p_decal) {
		const Decal *decal = decal_owner.get_or_null(p_decal);
		return decal->cull_mask;
	}

	_FORCE_INLINE_ float decal_get_upper_fade(RID p_decal) {
		const Decal *decal = decal_owner.get_or_null(p_decal);
		return decal->upper_fade;
	}

	_FORCE_INLINE_ float decal_get_lower_fade(RID p_decal) {
		const Decal *decal = decal_owner.get_or_null(p_decal);
		return decal->lower_fade;
	}

	_FORCE_INLINE_ float decal_get_normal_fade(RID p_decal) {
		const Decal *decal = decal_owner.get_or_null(p_decal);
		return decal->normal_fade;
	}

	_FORCE_INLINE_ bool decal_is_distance_fade_enabled(RID p_decal) {
		const Decal *decal = decal_owner.get_or_null(p_decal);
		return decal->distance_fade;
	}

	_FORCE_INLINE_ float decal_get_distance_fade_begin(RID p_decal) {
		const Decal *decal = decal_owner.get_or_null(p_decal);
		return decal->distance_fade_begin;
	}

	_FORCE_INLINE_ float decal_get_distance_fade_length(RID p_decal) {
		const Decal *decal = decal_owner.get_or_null(p_decal);
		return decal->distance_fade_length;
	}

	virtual AABB decal_get_aabb(RID p_decal) const override;

	/* VOXEL GI API */

	RID voxel_gi_allocate() override;
	void voxel_gi_initialize(RID p_voxel_gi) override;

	void voxel_gi_allocate_data(RID p_voxel_gi, const Transform3D &p_to_cell_xform, const AABB &p_aabb, const Vector3i &p_octree_size, const Vector<uint8_t> &p_octree_cells, const Vector<uint8_t> &p_data_cells, const Vector<uint8_t> &p_distance_field, const Vector<int> &p_level_counts) override;

	AABB voxel_gi_get_bounds(RID p_voxel_gi) const override;
	Vector3i voxel_gi_get_octree_size(RID p_voxel_gi) const override;
	Vector<uint8_t> voxel_gi_get_octree_cells(RID p_voxel_gi) const override;
	Vector<uint8_t> voxel_gi_get_data_cells(RID p_voxel_gi) const override;
	Vector<uint8_t> voxel_gi_get_distance_field(RID p_voxel_gi) const override;

	Vector<int> voxel_gi_get_level_counts(RID p_voxel_gi) const override;
	Transform3D voxel_gi_get_to_cell_xform(RID p_voxel_gi) const override;

	void voxel_gi_set_dynamic_range(RID p_voxel_gi, float p_range) override;
	float voxel_gi_get_dynamic_range(RID p_voxel_gi) const override;

	void voxel_gi_set_propagation(RID p_voxel_gi, float p_range) override;
	float voxel_gi_get_propagation(RID p_voxel_gi) const override;

	void voxel_gi_set_energy(RID p_voxel_gi, float p_energy) override;
	float voxel_gi_get_energy(RID p_voxel_gi) const override;

	void voxel_gi_set_bias(RID p_voxel_gi, float p_bias) override;
	float voxel_gi_get_bias(RID p_voxel_gi) const override;

	void voxel_gi_set_normal_bias(RID p_voxel_gi, float p_range) override;
	float voxel_gi_get_normal_bias(RID p_voxel_gi) const override;

	void voxel_gi_set_interior(RID p_voxel_gi, bool p_enable) override;
	bool voxel_gi_is_interior(RID p_voxel_gi) const override;

	void voxel_gi_set_use_two_bounces(RID p_voxel_gi, bool p_enable) override;
	bool voxel_gi_is_using_two_bounces(RID p_voxel_gi) const override;

	void voxel_gi_set_anisotropy_strength(RID p_voxel_gi, float p_strength) override;
	float voxel_gi_get_anisotropy_strength(RID p_voxel_gi) const override;

	uint32_t voxel_gi_get_version(RID p_probe) override;
	uint32_t voxel_gi_get_data_version(RID p_probe);

	RID voxel_gi_get_octree_buffer(RID p_voxel_gi) const;
	RID voxel_gi_get_data_buffer(RID p_voxel_gi) const;

	RID voxel_gi_get_sdf_texture(RID p_voxel_gi);

	/* LIGHTMAP CAPTURE */

	RID lightmap_allocate() override;
	void lightmap_initialize(RID p_lightmap) override;

	virtual void lightmap_set_textures(RID p_lightmap, RID p_light, bool p_uses_spherical_haromics) override;
	virtual void lightmap_set_probe_bounds(RID p_lightmap, const AABB &p_bounds) override;
	virtual void lightmap_set_probe_interior(RID p_lightmap, bool p_interior) override;
	virtual void lightmap_set_probe_capture_data(RID p_lightmap, const PackedVector3Array &p_points, const PackedColorArray &p_point_sh, const PackedInt32Array &p_tetrahedra, const PackedInt32Array &p_bsp_tree) override;
	virtual PackedVector3Array lightmap_get_probe_capture_points(RID p_lightmap) const override;
	virtual PackedColorArray lightmap_get_probe_capture_sh(RID p_lightmap) const override;
	virtual PackedInt32Array lightmap_get_probe_capture_tetrahedra(RID p_lightmap) const override;
	virtual PackedInt32Array lightmap_get_probe_capture_bsp_tree(RID p_lightmap) const override;
	virtual AABB lightmap_get_aabb(RID p_lightmap) const override;
	virtual bool lightmap_is_interior(RID p_lightmap) const override;
	virtual void lightmap_tap_sh_light(RID p_lightmap, const Vector3 &p_point, Color *r_sh) override;
	virtual void lightmap_set_probe_capture_update_speed(float p_speed) override;
	float lightmap_get_probe_capture_update_speed() const override {
		return lightmap_probe_capture_update_speed;
	}
	_FORCE_INLINE_ RID lightmap_get_texture(RID p_lightmap) const {
		const Lightmap *lm = lightmap_owner.get_or_null(p_lightmap);
		ERR_FAIL_COND_V(!lm, RID());
		return lm->light_texture;
	}
	_FORCE_INLINE_ int32_t lightmap_get_array_index(RID p_lightmap) const {
		ERR_FAIL_COND_V(!using_lightmap_array, -1); //only for arrays
		const Lightmap *lm = lightmap_owner.get_or_null(p_lightmap);
		return lm->array_index;
	}
	_FORCE_INLINE_ bool lightmap_uses_spherical_harmonics(RID p_lightmap) const {
		ERR_FAIL_COND_V(!using_lightmap_array, false); //only for arrays
		const Lightmap *lm = lightmap_owner.get_or_null(p_lightmap);
		return lm->uses_spherical_harmonics;
	}
	_FORCE_INLINE_ uint64_t lightmap_array_get_version() const {
		ERR_FAIL_COND_V(!using_lightmap_array, 0); //only for arrays
		return lightmap_array_version;
	}

	_FORCE_INLINE_ int lightmap_array_get_size() const {
		ERR_FAIL_COND_V(!using_lightmap_array, 0); //only for arrays
		return lightmap_textures.size();
	}

	_FORCE_INLINE_ const Vector<RID> &lightmap_array_get_textures() const {
		ERR_FAIL_COND_V(!using_lightmap_array, lightmap_textures); //only for arrays
		return lightmap_textures;
	}

	/* PARTICLES */

	RID particles_allocate() override;
	void particles_initialize(RID p_particles_collision) override;

	void particles_set_mode(RID p_particles, RS::ParticlesMode p_mode) override;
	void particles_set_emitting(RID p_particles, bool p_emitting) override;
	void particles_set_amount(RID p_particles, int p_amount) override;
	void particles_set_lifetime(RID p_particles, double p_lifetime) override;
	void particles_set_one_shot(RID p_particles, bool p_one_shot) override;
	void particles_set_pre_process_time(RID p_particles, double p_time) override;
	void particles_set_explosiveness_ratio(RID p_particles, real_t p_ratio) override;
	void particles_set_randomness_ratio(RID p_particles, real_t p_ratio) override;
	void particles_set_custom_aabb(RID p_particles, const AABB &p_aabb) override;
	void particles_set_speed_scale(RID p_particles, double p_scale) override;
	void particles_set_use_local_coordinates(RID p_particles, bool p_enable) override;
	void particles_set_process_material(RID p_particles, RID p_material) override;
	RID particles_get_process_material(RID p_particles) const override;

	void particles_set_fixed_fps(RID p_particles, int p_fps) override;
	void particles_set_interpolate(RID p_particles, bool p_enable) override;
	void particles_set_fractional_delta(RID p_particles, bool p_enable) override;
	void particles_set_collision_base_size(RID p_particles, real_t p_size) override;
	void particles_set_transform_align(RID p_particles, RS::ParticlesTransformAlign p_transform_align) override;

	void particles_set_trails(RID p_particles, bool p_enable, double p_length) override;
	void particles_set_trail_bind_poses(RID p_particles, const Vector<Transform3D> &p_bind_poses) override;

	void particles_restart(RID p_particles) override;
	void particles_emit(RID p_particles, const Transform3D &p_transform, const Vector3 &p_velocity, const Color &p_color, const Color &p_custom, uint32_t p_emit_flags) override;

	void particles_set_subemitter(RID p_particles, RID p_subemitter_particles) override;

	void particles_set_draw_order(RID p_particles, RS::ParticlesDrawOrder p_order) override;

	void particles_set_draw_passes(RID p_particles, int p_count) override;
	void particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh) override;

	void particles_request_process(RID p_particles) override;
	AABB particles_get_current_aabb(RID p_particles) override;
	AABB particles_get_aabb(RID p_particles) const override;

	void particles_set_emission_transform(RID p_particles, const Transform3D &p_transform) override;

	bool particles_get_emitting(RID p_particles) override;
	int particles_get_draw_passes(RID p_particles) const override;
	RID particles_get_draw_pass_mesh(RID p_particles, int p_pass) const override;

	void particles_set_view_axis(RID p_particles, const Vector3 &p_axis, const Vector3 &p_up_axis) override;

	virtual bool particles_is_inactive(RID p_particles) const override;

	_FORCE_INLINE_ RS::ParticlesMode particles_get_mode(RID p_particles) {
		Particles *particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_COND_V(!particles, RS::PARTICLES_MODE_2D);
		return particles->mode;
	}

	_FORCE_INLINE_ uint32_t particles_get_amount(RID p_particles, uint32_t &r_trail_divisor) {
		Particles *particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_COND_V(!particles, 0);

		if (particles->trails_enabled && particles->trail_bind_poses.size() > 1) {
			r_trail_divisor = particles->trail_bind_poses.size();
		} else {
			r_trail_divisor = 1;
		}

		return particles->amount * r_trail_divisor;
	}

	_FORCE_INLINE_ bool particles_has_collision(RID p_particles) {
		Particles *particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_COND_V(!particles, 0);

		return particles->has_collision_cache;
	}

	_FORCE_INLINE_ uint32_t particles_is_using_local_coords(RID p_particles) {
		Particles *particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_COND_V(!particles, false);

		return particles->use_local_coords;
	}

	_FORCE_INLINE_ RID particles_get_instance_buffer_uniform_set(RID p_particles, RID p_shader, uint32_t p_set) {
		Particles *particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_COND_V(!particles, RID());
		if (particles->particles_transforms_buffer_uniform_set.is_null()) {
			_particles_update_buffers(particles);

			Vector<RD::Uniform> uniforms;

			{
				RD::Uniform u;
				u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
				u.binding = 0;
				u.append_id(particles->particle_instance_buffer);
				uniforms.push_back(u);
			}

			particles->particles_transforms_buffer_uniform_set = RD::get_singleton()->uniform_set_create(uniforms, p_shader, p_set);
		}

		return particles->particles_transforms_buffer_uniform_set;
	}

	virtual void particles_add_collision(RID p_particles, RID p_particles_collision_instance) override;
	virtual void particles_remove_collision(RID p_particles, RID p_particles_collision_instance) override;
	virtual void particles_set_canvas_sdf_collision(RID p_particles, bool p_enable, const Transform2D &p_xform, const Rect2 &p_to_screen, RID p_texture) override;

	/* PARTICLES COLLISION */

	RID particles_collision_allocate() override;
	void particles_collision_initialize(RID p_particles_collision) override;

	virtual void particles_collision_set_collision_type(RID p_particles_collision, RS::ParticlesCollisionType p_type) override;
	virtual void particles_collision_set_cull_mask(RID p_particles_collision, uint32_t p_cull_mask) override;
	virtual void particles_collision_set_sphere_radius(RID p_particles_collision, real_t p_radius) override; //for spheres
	virtual void particles_collision_set_box_extents(RID p_particles_collision, const Vector3 &p_extents) override; //for non-spheres
	virtual void particles_collision_set_attractor_strength(RID p_particles_collision, real_t p_strength) override;
	virtual void particles_collision_set_attractor_directionality(RID p_particles_collision, real_t p_directionality) override;
	virtual void particles_collision_set_attractor_attenuation(RID p_particles_collision, real_t p_curve) override;
	virtual void particles_collision_set_field_texture(RID p_particles_collision, RID p_texture) override; //for SDF and vector field, heightfield is dynamic
	virtual void particles_collision_height_field_update(RID p_particles_collision) override; //for SDF and vector field
	virtual void particles_collision_set_height_field_resolution(RID p_particles_collision, RS::ParticlesCollisionHeightfieldResolution p_resolution) override; //for SDF and vector field
	virtual AABB particles_collision_get_aabb(RID p_particles_collision) const override;
	virtual Vector3 particles_collision_get_extents(RID p_particles_collision) const;
	virtual bool particles_collision_is_heightfield(RID p_particles_collision) const override;
	RID particles_collision_get_heightfield_framebuffer(RID p_particles_collision) const override;

	/* FOG VOLUMES */

	virtual RID fog_volume_allocate() override;
	virtual void fog_volume_initialize(RID p_rid) override;

	virtual void fog_volume_set_shape(RID p_fog_volume, RS::FogVolumeShape p_shape) override;
	virtual void fog_volume_set_extents(RID p_fog_volume, const Vector3 &p_extents) override;
	virtual void fog_volume_set_material(RID p_fog_volume, RID p_material) override;
	virtual RS::FogVolumeShape fog_volume_get_shape(RID p_fog_volume) const override;
	virtual RID fog_volume_get_material(RID p_fog_volume) const;
	virtual AABB fog_volume_get_aabb(RID p_fog_volume) const override;
	virtual Vector3 fog_volume_get_extents(RID p_fog_volume) const;

	/* VISIBILITY NOTIFIER */

	virtual RID visibility_notifier_allocate() override;
	virtual void visibility_notifier_initialize(RID p_notifier) override;
	virtual void visibility_notifier_set_aabb(RID p_notifier, const AABB &p_aabb) override;
	virtual void visibility_notifier_set_callbacks(RID p_notifier, const Callable &p_enter_callbable, const Callable &p_exit_callable) override;

	virtual AABB visibility_notifier_get_aabb(RID p_notifier) const override;
	virtual void visibility_notifier_call(RID p_notifier, bool p_enter, bool p_deferred) override;

	//used from 2D and 3D
	virtual RID particles_collision_instance_create(RID p_collision) override;
	virtual void particles_collision_instance_set_transform(RID p_collision_instance, const Transform3D &p_transform) override;
	virtual void particles_collision_instance_set_active(RID p_collision_instance, bool p_active) override;

	/* GLOBAL VARIABLES API */

	virtual void global_variable_add(const StringName &p_name, RS::GlobalVariableType p_type, const Variant &p_value) override;
	virtual void global_variable_remove(const StringName &p_name) override;
	virtual Vector<StringName> global_variable_get_list() const override;

	virtual void global_variable_set(const StringName &p_name, const Variant &p_value) override;
	virtual void global_variable_set_override(const StringName &p_name, const Variant &p_value) override;
	virtual Variant global_variable_get(const StringName &p_name) const override;
	virtual RS::GlobalVariableType global_variable_get_type(const StringName &p_name) const override;
	RS::GlobalVariableType global_variable_get_type_internal(const StringName &p_name) const;

	virtual void global_variables_load_settings(bool p_load_textures = true) override;
	virtual void global_variables_clear() override;

	virtual int32_t global_variables_instance_allocate(RID p_instance) override;
	virtual void global_variables_instance_free(RID p_instance) override;
	virtual void global_variables_instance_update(RID p_instance, int p_index, const Variant &p_value) override;

	RID global_variables_get_storage_buffer() const;

	/* RENDER TARGET API */

	RID render_target_create() override;
	void render_target_set_position(RID p_render_target, int p_x, int p_y) override;
	void render_target_set_size(RID p_render_target, int p_width, int p_height, uint32_t p_view_count) override;
	RID render_target_get_texture(RID p_render_target) override;
	void render_target_set_external_texture(RID p_render_target, unsigned int p_texture_id) override;
	void render_target_set_flag(RID p_render_target, RenderTargetFlags p_flag, bool p_value) override;
	bool render_target_was_used(RID p_render_target) override;
	void render_target_set_as_unused(RID p_render_target) override;
	void render_target_copy_to_back_buffer(RID p_render_target, const Rect2i &p_region, bool p_gen_mipmaps);
	void render_target_clear_back_buffer(RID p_render_target, const Rect2i &p_region, const Color &p_color);
	void render_target_gen_back_buffer_mipmaps(RID p_render_target, const Rect2i &p_region);

	RID render_target_get_back_buffer_uniform_set(RID p_render_target, RID p_base_shader);

	virtual void render_target_request_clear(RID p_render_target, const Color &p_clear_color) override;
	virtual bool render_target_is_clear_requested(RID p_render_target) override;
	virtual Color render_target_get_clear_request_color(RID p_render_target) override;
	virtual void render_target_disable_clear_request(RID p_render_target) override;
	virtual void render_target_do_clear_request(RID p_render_target) override;

	virtual void render_target_set_sdf_size_and_scale(RID p_render_target, RS::ViewportSDFOversize p_size, RS::ViewportSDFScale p_scale) override;
	RID render_target_get_sdf_texture(RID p_render_target);
	RID render_target_get_sdf_framebuffer(RID p_render_target);
	void render_target_sdf_process(RID p_render_target);
	virtual Rect2i render_target_get_sdf_rect(RID p_render_target) const override;
	void render_target_mark_sdf_enabled(RID p_render_target, bool p_enabled) override;
	bool render_target_is_sdf_enabled(RID p_render_target) const;

	Size2 render_target_get_size(RID p_render_target);
	RID render_target_get_rd_framebuffer(RID p_render_target);
	RID render_target_get_rd_texture(RID p_render_target);
	RID render_target_get_rd_backbuffer(RID p_render_target);
	RID render_target_get_rd_backbuffer_framebuffer(RID p_render_target);

	RID render_target_get_framebuffer_uniform_set(RID p_render_target);
	RID render_target_get_backbuffer_uniform_set(RID p_render_target);

	void render_target_set_framebuffer_uniform_set(RID p_render_target, RID p_uniform_set);
	void render_target_set_backbuffer_uniform_set(RID p_render_target, RID p_uniform_set);

	RS::InstanceType get_base_type(RID p_rid) const override;

	bool free(RID p_rid) override;

	bool has_os_feature(const String &p_feature) const override;

	void update_dirty_resources() override;

	void set_debug_generate_wireframes(bool p_generate) override {}

	//keep cached since it can be called form any thread
	uint64_t texture_mem_cache = 0;
	uint64_t buffer_mem_cache = 0;
	uint64_t total_mem_cache = 0;

	virtual void update_memory_info() override;
	virtual uint64_t get_rendering_info(RS::RenderingInfo p_info) override;

	String get_video_adapter_name() const override;
	String get_video_adapter_vendor() const override;
	RenderingDevice::DeviceType get_video_adapter_type() const override;

	virtual void capture_timestamps_begin() override;
	virtual void capture_timestamp(const String &p_name) override;
	virtual uint32_t get_captured_timestamps_count() const override;
	virtual uint64_t get_captured_timestamps_frame() const override;
	virtual uint64_t get_captured_timestamp_gpu_time(uint32_t p_index) const override;
	virtual uint64_t get_captured_timestamp_cpu_time(uint32_t p_index) const override;
	virtual String get_captured_timestamp_name(uint32_t p_index) const override;

	RID get_default_rd_storage_buffer() { return default_rd_storage_buffer; }

	static RendererStorageRD *base_singleton;

	void init_effects(bool p_prefer_raster_effects);
	EffectsRD *get_effects();

	RendererStorageRD();
	~RendererStorageRD();
};

#endif // RASTERIZER_STORAGE_RD_H
