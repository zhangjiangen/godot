/*************************************************************************/
/*  rendering_device_binds.cpp                                           */
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

#include "rendering_device_binds.h"
#include "core/string/string_builder.h"
#include "core/variant/variant_parser.h"
// gpu 机构提的主体模版
static const char *gpu_struct_gd_script_template = "\
#AUTO_GENERATED_BODY_START\
extends Resource \
class_name %s \
var _property_dict:Dictionary = {} \
var _buffer : PackedByteArray = PackedByteArray() \
var _gpu_struct_instance:UniformBufferMemoberStructInstance = null \
# export property list start \
%s \
# export property list end \
static func build_gpu_struct(): \
	var gpu_struct = RenderingServer.get_gpu_struct() \
	if gpu_struct != null: \
		if gpu_struct.struct_var == get_gpu_struct_version(): \
			return  \
	if gpu_struct == null: \
		gpu_struct = UniformBufferMemoberStruct.new(get_gpu_struct_name()) \
	else:\
		gpu_struct.clear() \
%s\
	gpu_struct.struct_var = get_gpu_struct_version() \
	gpu_struct.finish_decl() \
	gpu_struct._cb_update_check = get_gpu_struct\
static func get_gpu_struct_name(): \
	return \"%s\" \
static func get_gpu_struct_version()\
	return %d\
static func get_gpu_struct():\
	build_gpu_struct()\
	return RenderingServer.get_gpu_struct(get_gpu_struct_name())\
static func create_gpu_struct_instance():\
	return get_gpu_struct().instance()\
func _init():\
	_gpu_struct_instance = create_gpu_struct_instance() \
func update_gpu_buffer(p_byte_offset:int,p_uniform_buffer: PackedByteArray): \
	_gpu_struct_instance._updata_buffer(p_byte_offset,p_uniform_buffer,_property_dict) \
#AUTO_GENERATED_BODY_END\
";
// gpu结构体的成员定义模版
const char *gpu_struct_gd_script_struct_member_decl_template = "\
	gpu_struct.add_struct_member(%s,%s,%d,%s,%s) \n\
";
const char *gpu_struct_gd_simple_struct_member_decl_template = "\
	gpu_struct.add_member(%s,%s,%d,%s,%s) \n\
";
static void gpu_struct_member_to_gd_script_export(StringBuilder &sb, const String &name, const Variant::Type &type, const String &default_value, bool is_struct, const String &struct_name, bool is_range, const Vector2 &range) {
	sb.append("@export");
	if (is_range) {
		if (type == Variant::FLOAT || type == Variant::INT) {
			sb.append("_range(");
			sb.append(rtos(range.x));
			sb.append(",");
			sb.append(rtos(range.y));
			sb.append(")");
		}
	}
	sb.append(" ");
	sb.append(name);
	sb.append(" : ");
	if (is_struct) {
		sb.append("UniformBufferMemoberStructInstance");
	} else {
		sb.append(Variant::get_type_name(type));
	}
	sb.append("\n");
	sb.append("\tget: \n\t\t");
	if (is_struct) {
		sb.append("if !_property_dict.has(");
		sb.append(name);
		sb.append("): \n\t\t\t_property_dict[\"");
		sb.append(name);
		sb.append("\"] = ");
		sb.append(struct_name);
		sb.append(".create_gpu_struct_instance() \n");
		sb.append("\t\treturn _property_dict[\"");
		sb.append(name);
		sb.append("\"] \n\t\t");

		sb.append("\tset(v): \n\t\t\t");
		sb.append("_property_dict[\"");
		sb.append(name);
		sb.append("\"] = v \n");
	} else {
		sb.append("if !_property_dict.has(");
		sb.append(name);
		sb.append("): \n\t\t\t_property_dict[\"");
		sb.append(name);
		sb.append("\"] = ");
		sb.append(default_value);
		sb.append("\t\t");
		sb.append("return _property_dict[\"");
		sb.append(name);
		sb.append("\"] \n\t\t");

		sb.append("\tset(v): \n\t\t\t");
		sb.append("_property_dict[\"");
		sb.append(name);
		sb.append("\"] = v \n");
	}
}
// gpu 结构体转换成gdscript
static bool gpu_struct_to_gdscript(const String &struct_name, Dictionary gpu_struct_dict, String &p_gd_script, int version) {
	if (gpu_struct_dict.has("error")) {
		return false;
	}
	String gd_script;
	StringBuilder gd_script_member_decl;
	StringBuilder gd_script_member_export;
	List<Variant> member_list;
	char temp_str[2048];
	gpu_struct_dict.get_key_list(&member_list);
	for (int i = 0; i < member_list.size(); i++) {
		Array grroup = gpu_struct_dict[member_list[i]];
		for (int j = 0; j < grroup.size(); j++) {
			// 处理成员定义
			Dictionary member_dict = grroup[j];
			int array_count = member_dict["array_count"];
			String name = member_dict["name"];
			Dictionary hint = member_dict["hint"];
			bool is_globle = false;
			bool is_single = false;
			bool is_range = false;
			Vector2 range;
			if (hint.has("global")) {
				is_globle = hint["global"];
			}
			if (hint.has("singgle")) {
				is_single = hint["single"];
			}
			if (hint.has("range")) {
				is_range = true;
				range = hint["range"];
			}

			if (member_dict.has("struct_name")) {
				String struct_name = member_dict["struct_name"];
				// 结构体成员
				snprintf(temp_str, sizeof(temp_str), gpu_struct_gd_simple_struct_member_decl_template, struct_name.utf8().get_data(), name.utf8().get_data(), array_count,
						is_single ? "true" : "false", is_globle ? "true" : "false");
				gd_script_member_decl + temp_str;
				gpu_struct_member_to_gd_script_export(gd_script_member_export, name, Variant::NIL, "", false, struct_name, is_range, range);

			} else {
				int type = member_dict["type"];
				// 普通成员
				String def_value;
				VariantWriter::write_to_string(member_dict["default_value"], def_value);
				snprintf(temp_str, sizeof(temp_str), gpu_struct_gd_simple_struct_member_decl_template, Variant::variant_type_to_gdscriot_type( (Variant::Type &)type).utf8().get_data(), name.utf8().get_data(),
						def_value.utf8().get_data(), array_count, is_single ? "true" : "false", is_globle ? "true" : "false");
				gd_script_member_decl + temp_str;
				gpu_struct_member_to_gd_script_export(gd_script_member_export, name, (Variant::Type &)type, def_value, false, "", is_range, range);
			}
		}
	}

	// 合并最终主体
	snprintf(temp_str, sizeof(temp_str), gpu_struct_gd_script_template, struct_name.utf8().get_data(), gd_script_member_export.as_string().utf8().get_data(), gd_script_member_decl.as_string().utf8().get_data(), struct_name.utf8().get_data(), version);
	gd_script = temp_str;
	return true;
}
// 保存gpu结构体到gdscript脚本文件
static void gpu_struct_saveto_gdscript_dict(const String &p_path, Dictionary gpu_struct_dict, int version) {
	if (gpu_struct_dict.has("error")) {
		return;
	}

	List<Variant> struct_list;
	gpu_struct_dict.get_key_list(&struct_list);
	for (int i = 0; i < struct_list.size(); i++) {
		Dictionary gpu_struct = gpu_struct_dict[struct_list[i]];

		Dictionary ret;
		String gd_script;
		if (gpu_struct_to_gdscript(struct_list[i], gpu_struct_dict, gd_script, version)) {
            String struct_name =struct_list[i];
			String file_path = p_path + "/" + struct_name + ".gd";
			if (FileAccess::exists(file_path)) {
				String old_gd_script = FileAccess::get_file_as_string(file_path);
				int start = old_gd_script.find("#AUTO_GENERATED_BODY_START");
				int end = old_gd_script.find("#AUTO_GENERATED_BODY_END");
				if (start >= 0 && end >= 0) {
					String begin_str = old_gd_script.substr(0, start);
					String end_str = old_gd_script.substr(end + strlen("#AUTO_GENERATED_BODY_END"));
					begin_str += gd_script;
					begin_str += end_str;
					// 删除旧的文件
					FileAccess::delete_file(file_path);
					FileAccess::save_string(file_path, begin_str);
				} else {
					FileAccess::save_string(file_path, gd_script);
				}
			}
		}
	}
}

void RDShaderFile::save_gpu_structs_to_gd_script(const String &p_path) {
	Dictionary gpu_struct_dict = get_gpu_structs();
	gpu_struct_saveto_gdscript_dict(p_path, gpu_struct_dict, get_gpu_structs_version());
}
Error RDShaderFile::parse_versions_from_text(const String &p_text, const String p_defines, OpenIncludeFunction p_include_func, void *p_include_func_userdata) {
	Vector<String> lines = p_text.remove_commentary().split("\n");

	bool reading_versions = false;
	bool stage_found[RD::SHADER_STAGE_MAX] = { false, false, false, false, false };
	RD::ShaderStage stage = RD::SHADER_STAGE_MAX;
	static const char *stage_str[RD::SHADER_STAGE_MAX] = {
		"vertex",
		"fragment",
		"tesselation_control",
		"tesselation_evaluation",
		"compute",
		"task",
		"mesh"
	};
	String stage_code[RD::SHADER_STAGE_MAX];
	int stages_found = 0;
	HashMap<StringName, String> version_texts;

	versions.clear();
	base_error = "";

	for (int lidx = 0; lidx < lines.size(); lidx++) {
		String line = lines[lidx];

		{
			String ls = line.strip_edges();
			if (ls.begins_with("#[") && ls.ends_with("]")) {
				String section = ls.substr(2, ls.length() - 3).strip_edges();
				if (section == "versions") {
					if (stages_found) {
						base_error = "Invalid shader file, #[versions] must be the first section found.";
						break;
					}
					reading_versions = true;
				} else {
					for (int i = 0; i < RD::SHADER_STAGE_MAX; i++) {
						if (section == stage_str[i]) {
							if (stage_found[i]) {
								base_error = "Invalid shader file, stage appears twice: " + section;
								break;
							}

							stage_found[i] = true;
							stages_found++;

							stage = RD::ShaderStage(i);
							reading_versions = false;
							break;
						}
					}

					if (!base_error.is_empty()) {
						break;
					}
				}

				continue;
			}
		}

		if (stage == RD::SHADER_STAGE_MAX && !line.strip_edges().is_empty()) {
			line = line.strip_edges();
			if (line.begins_with("//") || line.begins_with("/*")) {
				if (line.begins_with("//")) {
					line = "\n";
				}
				continue; //assuming comment (single line)
			}
		}

		if (reading_versions) {
			String l = line.strip_edges();
			if (!l.is_empty()) {
				if (!l.contains("=")) {
					base_error = "Missing `=` in '" + l + "'. Version syntax is `version = \"<defines with C escaping>\";`.";
					break;
				}
				if (!l.contains(";")) {
					// We don't require a semicolon per se, but it's needed for clang-format to handle things properly.
					base_error = "Missing `;` in '" + l + "'. Version syntax is `version = \"<defines with C escaping>\";`.";
					break;
				}
				Vector<String> slices = l.get_slice(";", 0).split("=");
				String version = slices[0].strip_edges();
				if (!version.is_valid_identifier()) {
					base_error = "Version names must be valid identifiers, found '" + version + "' instead.";
					break;
				}
				String define = slices[1].strip_edges();
				if (!define.begins_with("\"") || !define.ends_with("\"")) {
					base_error = "Version text must be quoted using \"\", instead found '" + define + "'.";
					break;
				}
				define = "\n" + define.substr(1, define.length() - 2).c_unescape() + "\n"; // Add newline before and after just in case.

				version_texts[version] = define + "\n" + p_defines;
			}
		} else {
			if (stage == RD::SHADER_STAGE_MAX && !line.strip_edges().is_empty()) {
				base_error = "Text was found that does not belong to a valid section: " + line;
				break;
			}

			if (stage != RD::SHADER_STAGE_MAX) {
				if (line.strip_edges().begins_with("#include")) {
					if (p_include_func) {
						//process include
						String include = line.replace("#include", "").strip_edges();
						if (!include.begins_with("\"") || !include.ends_with("\"")) {
							base_error = "Malformed #include syntax, expected #include \"<path>\", found instead: " + include;
							break;
						}
						include = include.substr(1, include.length() - 2).strip_edges();
						String include_text = p_include_func(include, p_include_func_userdata).remove_commentary();
						if (!include_text.is_empty()) {
							stage_code[stage] += "\n" + include_text + "\n";
						} else {
							base_error = "#include failed for file '" + include + "'";
						}
					} else {
						base_error = "#include used, but no include function provided.";
					}
				} else {
					stage_code[stage] += line + "\n";
				}
			}
		}
	}

	bool is_init_struct = false;

	if (base_error.is_empty()) {
		if (stage_found[RD::SHADER_STAGE_COMPUTE] && stages_found > 1) {
			ERR_FAIL_V_MSG(ERR_PARSE_ERROR, "When writing compute shaders, [compute] mustbe the only stage present.");
		}

		if (version_texts.is_empty()) {
			version_texts[""] = ""; //make sure a default version exists
		}

		bool errors_found = false;

		/* STEP 2, Compile the versions, add to shader file */

		for (const KeyValue<StringName, String> &E : version_texts) {
			Ref<RDShaderSPIRV> bytecode;
			New_instantiate(bytecode);

			for (int i = 0; i < RD::SHADER_STAGE_MAX; i++) {
				String code = stage_code[i];
				if (code.is_empty()) {
					continue;
				}
				code = code.replace("VERSION_DEFINES", E.value);
				String error;
				Vector<uint8_t> spirv = RenderingDevice::get_singleton()->shader_compile_spirv_from_source(RD::ShaderStage(i), code, RD::SHADER_LANGUAGE_GLSL, &error, false);
				bytecode->set_stage_bytecode(RD::ShaderStage(i), spirv);
				if (!error.is_empty()) {
					error += String() + "\n\nStage '" + stage_str[i] + "' source code: \n\n";
					Vector<String> sclines = code.split("\n");
					for (int j = 0; j < sclines.size(); j++) {
						error += itos(j + 1) + "\t\t" + sclines[j] + "\n";
					}
					errors_found = true;
				}
				bytecode->set_stage_compile_error(RD::ShaderStage(i), error);
				// 解析结构体信息
				if (!is_init_struct && !errors_found) {
					gpu_structs = code.parse_gpu_struct();
					++gpu_structs_version;
					is_init_struct = true;
				}
			}

			set_bytecode(bytecode, E.key);
		}

		return errors_found ? ERR_PARSE_ERROR : OK;
	} else {
		return ERR_PARSE_ERROR;
	}
}
