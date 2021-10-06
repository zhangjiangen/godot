/*************************************************************************/
/*  shader_rd.h                                                          */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
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

#ifndef SHADER_RD_H
#define SHADER_RD_H

#include "core/os/mutex.h"
#include "core/string/string_builder.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "core/templates/map.h"
#include "core/templates/rid_owner.h"
#include "core/variant/variant.h"
#include "servers/rendering_server.h"

#include <stdio.h>
/**
	@author Juan Linietsky <reduzio@gmail.com>
*/

class ShaderRD {
	//versions
	CharString general_defines;
	Vector<CharString> variant_defines;
	Vector<bool> variants_enabled;

	struct Version {
		CharString uniforms;
		CharString vertex_globals;
		CharString compute_globals;
		CharString fragment_globals;
		CharString tesc_globals;
		CharString tese_globals;
		Map<StringName, CharString> code_sections;
		Vector<CharString> custom_defines;
		String GetString() {
			String ret;
			for (int i = 0; i < custom_defines.size(); ++i) {
				ret += custom_defines[i] + "\n";
			}
			return ret;
		}

		Vector<uint8_t> *variant_data = nullptr;
		RID *variants = nullptr; //same size as version defines

		bool valid;
		bool dirty;
		bool initialize_needed;
	};

	Mutex variant_set_mutex;

	void _compile_variant(uint32_t p_variant, Version *p_version);

	void _clear_version(Version *p_version);
	void _compile_version(Version *p_version);

	RID_Owner<Version> version_owner;

	struct StageTemplate {
		struct Chunk {
			enum Type {
				TYPE_VERSION_DEFINES,
				TYPE_MATERIAL_UNIFORMS,
				TYPE_VERTEX_GLOBALS,
				TYPE_FRAGMENT_GLOBALS,
				TYPE_TESSELATION_CONTROL_GLOBALS,
				TYPE_TESSELLATION_EVALUATION_GLOBALS,
				TYPE_COMPUTE_GLOBALS,
				TYPE_CODE,
				TYPE_TEXT
			};

			Type type;
			StringName code;
			CharString text;
		};
		LocalVector<Chunk> chunks;
	};

	bool is_compute = false;

	String name;

	CharString base_compute_defines;

	String base_sha256;

	static String shader_cache_dir;
	static bool shader_cache_cleanup_on_start;
	static bool shader_cache_save_compressed;
	static bool shader_cache_save_compressed_zstd;
	static bool shader_cache_save_debug;
	bool shader_cache_dir_valid = false;

	bool use_tess = false;

	enum StageType {
		STAGE_TYPE_VERTEX,
		STAGE_TYPE_FRAGMENT,
		STAGE_TYPE_TESSELATION_CONTROL,
		STAGE_TYPE_TESSELLATION_EVALUATION,
		STAGE_TYPE_COMPUTE,
		STAGE_TYPE_MAX,
	};

	StageTemplate stage_templates[STAGE_TYPE_MAX];

	void _build_variant_code(StringBuilder &p_builder, uint32_t p_variant, const Version *p_version, const StageTemplate &p_template);

	void _add_stage(const char *p_code, StageType p_stage_type);

	String _version_get_sha1(Version *p_version) const;
	bool _load_from_cache(Version *p_version);
	void _save_to_cache(Version *p_version);

protected:
	ShaderRD();
	void setup(const char *p_vertex_code, const char *p_fragment_code, const char *p_compute_code, const char *p_name, const char *p_tesc_code = nullptr, const char *p_tese_code = nullptr);

public:
	RID version_create();

	void version_set_code(RID p_version, const Map<String, String> &p_code, const String &p_uniforms, const String &p_vertex_globals, const String &p_fragment_globals, const Vector<String> &p_custom_defines);
	void version_set_compute_code(RID p_version, const Map<String, String> &p_code, const String &p_uniforms, const String &p_compute_globals, const Vector<String> &p_custom_defines);

	_FORCE_INLINE_ RID version_get_shader(RID p_version, int p_variant) {
		if (p_variant >= variant_defines.size()) {
			ERR_FAIL_INDEX_V(p_variant, variant_defines.size(), RID());
		}
		ERR_FAIL_COND_V(!variants_enabled[p_variant], RID());

		Version *version = version_owner.getornull(p_version);
		if (!version) {
			ERR_FAIL_COND_V(!version, RID());
		}

		if (version->dirty) {
			_compile_version(version);
		}

		if (!version->valid) {
			return RID();
		}

		return version->variants[p_variant];
	}

	bool version_is_valid(RID p_version);

	bool version_free(RID p_version);

	void set_variant_enabled(int p_variant, bool p_enabled);
	bool is_variant_enabled(int p_variant) const;

	static void set_shader_cache_dir(const String &p_dir);
	static void set_shader_cache_save_compressed(bool p_enable);
	static void set_shader_cache_save_compressed_zstd(bool p_enable);
	static void set_shader_cache_save_debug(bool p_enable);

	RS::ShaderNativeSourceCode version_get_native_source_code(RID p_version);

	void initialize(const Vector<String> &p_variant_defines, const String &p_general_defines = "");
	void clear();
	virtual ~ShaderRD();
};

#endif
