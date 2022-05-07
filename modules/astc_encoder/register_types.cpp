/*************************************************************************/
/*  register_types.cpp                                                   */
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

#include "register_types.h"

#include "Source/astcenccli_internal.h"
#include "core/io/image.h"
#include "core/os/os.h"
#include "servers/rendering_server.h"
#include "texture_astc.h"

struct compression_workload_gd {
	astcenc_context *context;
	astcenc_image *image;
	astcenc_swizzle swizzle;
	uint8_t *data_out;
	size_t data_len;
	astcenc_error error;
};
static void compression_workload_runner(int thread_count, int thread_id,
		void *payload) {
	compression_workload_gd *work = static_cast<compression_workload_gd *>(payload);
	astcenc_error error =
			astcenc_compress_image(work->context, work->image, &work->swizzle,
					work->data_out, work->data_len, thread_id);

	if (error != ASTCENC_SUCCESS) {
		work->error = error;
	}
}
// srgb 模式现在好像没法选
static void image_compress_astc_func(Image *r_img, float p_lossy_quality, Image::CompressMode compress_format, Image::UsedChannels p_channels, Image::CompressSource source) {
	Image::Format img_format = r_img->get_format();

	if (img_format >= Image::FORMAT_DXT1) {
		return; // Do not compress, already compressed.
	}
	if (img_format > Image::FORMAT_RGBA8) {
		// TODO: we should be able to handle FORMAT_RGBA4444 and FORMAT_RGBA5551 eventually
		return;
	}
	bool mipmaps = r_img->has_mipmaps();
	bool is_r = true;
	bool is_g = true;
	bool is_b = true;
	bool is_a = true;
	if (p_channels == Image::USED_CHANNELS_R || p_channels == Image::USED_CHANNELS_L) {
		is_g = false;
		is_b = false;
		is_a = false;
	} else if (p_channels == Image::USED_CHANNELS_RG) {
		is_b = false;
		is_a = false;
	} else if (p_channels == Image::USED_CHANNELS_RGB) {
		is_a = false;
	} else if (p_channels == Image::USED_CHANNELS_LA) {
		is_g = false;
		is_b = false;
	}

	astcenc_profile profile;
	astcenc_config config{};

	astcenc_error result;
	float preset = 0.0f;
#if defined(__arm__)
	preset = ASTCENC_PRE_FASTEST;
#elif TOOLS_ENABLED
	/** @brief The exhaustive, highest quality, search preset. */
	preset = ASTCENC_PRE_EXHAUSTIVE;
#else
	preset = ASTCENC_PRE_MEDIUM;
#endif
	Image::Format target_format = Image::FORMAT_RGBA_ASTC_4x4;
	if (source == Image::COMPRESS_SOURCE_SRGB)
		profile = ASTCENC_PRF_LDR_SRGB;
	else
		profile = ASTCENC_PRF_LDR;
	if (img_format > Image::FORMAT_SRGB8_ALPHA8_ASTC_4x4) {
		profile = ASTCENC_PRF_LDR_SRGB;
	}
	switch (compress_format) {
		case Image::COMPRESS_ASTC_4x4:
			result = astcenc_config_init(profile, 4, 4,
					1, preset, 0, &config);
			if (source == Image::COMPRESS_SOURCE_SRGB)
				target_format = Image::FORMAT_SRGB8_ALPHA8_ASTC_4x4;
			else
				target_format = Image::FORMAT_RGBA_ASTC_4x4;
			break;
		case Image::COMPRESS_ASTC_5x4:
			result = astcenc_config_init(profile, 5, 4,
					1, preset, 0, &config);
			if (source == Image::COMPRESS_SOURCE_SRGB)
				target_format = Image::FORMAT_SRGB8_ALPHA8_ASTC_5x4;
			else
				target_format = Image::FORMAT_RGBA_ASTC_5x4;
			break;
		case Image::COMPRESS_ASTC_5x5:
			result = astcenc_config_init(profile, 5, 5,
					1, preset, 0, &config);
			if (source == Image::COMPRESS_SOURCE_SRGB)
				target_format = Image::FORMAT_SRGB8_ALPHA8_ASTC_5x5;
			else
				target_format = Image::FORMAT_RGBA_ASTC_5x5;
			break;
		case Image::COMPRESS_ASTC_6x5:
			result = astcenc_config_init(profile, 6, 5,
					1, preset, 0, &config);
			if (source == Image::COMPRESS_SOURCE_SRGB)
				target_format = Image::FORMAT_SRGB8_ALPHA8_ASTC_6x5;
			else
				target_format = Image::FORMAT_RGBA_ASTC_6x5;
			break;
		case Image::COMPRESS_ASTC_6x6:
			result = astcenc_config_init(profile, 6, 6,
					1, preset, 0, &config);
			if (source == Image::COMPRESS_SOURCE_SRGB)
				target_format = Image::FORMAT_SRGB8_ALPHA8_ASTC_6x6;
			else
				target_format = Image::FORMAT_RGBA_ASTC_6x6;
			break;
		case Image::COMPRESS_ASTC_8x5:
			result = astcenc_config_init(profile, 8, 5,
					1, preset, 0, &config);
			if (source == Image::COMPRESS_SOURCE_SRGB)
				target_format = Image::FORMAT_SRGB8_ALPHA8_ASTC_8x5;
			else
				target_format = Image::FORMAT_RGBA_ASTC_8x5;
			break;
		case Image::COMPRESS_ASTC_8x6:
			result = astcenc_config_init(profile, 8, 6,
					1, preset, 0, &config);
			if (source == Image::COMPRESS_SOURCE_SRGB)
				target_format = Image::FORMAT_SRGB8_ALPHA8_ASTC_8x6;
			else
				target_format = Image::FORMAT_RGBA_ASTC_8x6;
			break;
		case Image::COMPRESS_ASTC_8x8:
			result = astcenc_config_init(profile, 8, 8,
					1, preset, 0, &config);
			if (source == Image::COMPRESS_SOURCE_SRGB)
				target_format = Image::FORMAT_SRGB8_ALPHA8_ASTC_8x8;
			else
				target_format = Image::FORMAT_RGBA_ASTC_8x8;
			break;
		case Image::COMPRESS_ASTC_10x5:
			result = astcenc_config_init(profile, 10, 5,
					1, preset, 0, &config);
			if (source == Image::COMPRESS_SOURCE_SRGB)
				target_format = Image::FORMAT_SRGB8_ALPHA8_ASTC_10x5;
			else
				target_format = Image::FORMAT_RGBA_ASTC_10x5;
			break;
		case Image::COMPRESS_ASTC_10x6:
			result = astcenc_config_init(profile, 10, 6,
					1, preset, 0, &config);
			if (source == Image::COMPRESS_SOURCE_SRGB)
				target_format = Image::FORMAT_SRGB8_ALPHA8_ASTC_10x6;
			else
				target_format = Image::FORMAT_RGBA_ASTC_10x6;
			break;
		case Image::COMPRESS_ASTC_10x8:
			result = astcenc_config_init(profile, 10, 8,
					1, preset, 0, &config);
			if (source == Image::COMPRESS_SOURCE_SRGB)
				target_format = Image::FORMAT_SRGB8_ALPHA8_ASTC_10x8;
			else
				target_format = Image::FORMAT_RGBA_ASTC_10x8;
			break;
		case Image::COMPRESS_ASTC_10x10:
			result = astcenc_config_init(profile, 10, 10,
					1, preset, 0, &config);
			if (source == Image::COMPRESS_SOURCE_SRGB)
				target_format = Image::FORMAT_SRGB8_ALPHA8_ASTC_10x10;
			else
				target_format = Image::FORMAT_RGBA_ASTC_10x10;
			break;
		case Image::COMPRESS_ASTC_12x10:
			result = astcenc_config_init(profile, 12, 10,
					1, preset, 0, &config);
			if (source == Image::COMPRESS_SOURCE_SRGB)
				target_format = Image::FORMAT_SRGB8_ALPHA8_ASTC_12x10;
			else
				target_format = Image::FORMAT_RGBA_ASTC_12x10;
			break;
		case Image::COMPRESS_ASTC_12x12:
			result = astcenc_config_init(profile, 12, 12,
					1, preset, 0, &config);
			if (source == Image::COMPRESS_SOURCE_SRGB)
				target_format = Image::FORMAT_SRGB8_ALPHA8_ASTC_12x12;
			else
				target_format = Image::FORMAT_RGBA_ASTC_12x12;
			break;
		default:
			return;
	}
	if (result != ASTCENC_SUCCESS) {
		return;
	}
	int threadcount = get_cpu_count();
	astcenc_context *codec_context;
	result = astcenc_context_alloc(&config, threadcount, &codec_context);
	if (result != ASTCENC_SUCCESS) {
		return;
	}
	// astc 目前只支持RGBA8格式
	if (img_format != Image::FORMAT_RGBA8) {
		r_img->convert(Image::FORMAT_RGBA8);
	}
	size_t buffer_size = Image::get_any_image_data_size(r_img->get_width(), r_img->get_height(), target_format, mipmaps);

	PackedByteArray data_buf;
	data_buf.resize(buffer_size);
	uint8_t *data_ptr = &data_buf.write[0];
	int os, mw, mh;
	astcenc_swizzle swz_encode;
	swz_encode.a = is_a ? ASTCENC_SWZ_A : ASTCENC_SWZ_1;
	swz_encode.r = is_r ? ASTCENC_SWZ_R : ASTCENC_SWZ_0;
	swz_encode.g = is_g ? ASTCENC_SWZ_G : ASTCENC_SWZ_0;
	swz_encode.b = is_b ? ASTCENC_SWZ_B : ASTCENC_SWZ_0;
	const uint8_t *src_read = r_img->get_data().ptr();
	//	uint32_t block_x = (width + block_width - 1) / block_width;
	int mip_count = Image::get_mipmap_count(r_img->get_width(), r_img->get_height(), target_format);
	if (mipmaps) {
		for (int i = 0; i < mip_count; ++i) {
			r_img->_get_mipmap_offset_and_size(i, os, mw, mh);
			astcenc_image *uncompressed_image = astc_img_from_unorm8x4_array(src_read + os, mw, mh, false);

			//
			compression_workload_gd work;
			work.context = codec_context;
			work.image = uncompressed_image;
			work.swizzle = swz_encode;
			work.data_out = data_ptr;
			work.data_len = buffer_size;
			work.error = ASTCENC_SUCCESS;
			launch_threads(threadcount, &compression_workload_runner, &work); // 输出的缓冲区的大小
			data_ptr += Image::get_any_image_data_size(mw, mh, target_format);

			free_image(uncompressed_image);
		}
	} else {
		astcenc_image *uncompressed_image = astc_img_from_unorm8x4_array(src_read, r_img->get_width(), r_img->get_height(), false);
		// 输出的缓冲区的大小

		//
		compression_workload_gd work;
		work.context = codec_context;
		work.image = uncompressed_image;
		work.swizzle = swz_encode;
		work.data_out = data_ptr;
		work.data_len = buffer_size;
		work.error = ASTCENC_SUCCESS;
		launch_threads(threadcount, &compression_workload_runner, &work);

		free_image(uncompressed_image);
	}
	astcenc_context_free(codec_context);
	// 下面保存astc
	r_img->create(r_img->get_width(), r_img->get_height(), mipmaps, target_format, data_buf);
	r_img->astc_a = is_a;
	r_img->astc_r = is_r;
	r_img->astc_g = is_g;
	r_img->astc_b = is_b;
}
static void image_decompress_astc(Image *r_img) {
	Image::Format img_format = r_img->get_format();

	if (img_format < Image::FORMAT_RGBA_ASTC_4x4) {
		return; // Do not compress, already compressed.
	}
	if (img_format > Image::FORMAT_SRGB8_ALPHA8_ASTC_12x12) {
		// TODO: we should be able to handle FORMAT_RGBA4444 and FORMAT_RGBA5551 eventually
		return;
	}
	bool mipmaps = r_img->has_mipmaps();
	astcenc_error result;
	astcenc_profile profile = ASTCENC_PRF_LDR;
	astcenc_config config{};
	static const float preset = ASTCENC_PRE_MEDIUM;
	switch (img_format) {
		case Image::FORMAT_RGBA_ASTC_4x4:
			result = astcenc_config_init(profile, 4, 4,
					1, preset, 0, &config);
			break;
		case Image::FORMAT_RGBA_ASTC_5x4:
			result = astcenc_config_init(profile, 5, 4,
					1, preset, 0, &config);
			break;
		case Image::FORMAT_RGBA_ASTC_5x5:
			result = astcenc_config_init(profile, 5, 5,
					1, preset, 0, &config);
			break;
		case Image::FORMAT_RGBA_ASTC_6x5:
			result = astcenc_config_init(profile, 6, 5,
					1, preset, 0, &config);
			break;
		case Image::FORMAT_RGBA_ASTC_6x6:
			result = astcenc_config_init(profile, 6, 6,
					1, preset, 0, &config);
			break;
		case Image::FORMAT_RGBA_ASTC_8x5:
			result = astcenc_config_init(profile, 8, 5,
					1, preset, 0, &config);
			break;
		case Image::FORMAT_RGBA_ASTC_8x6:
			result = astcenc_config_init(profile, 8, 6,
					1, preset, 0, &config);
			break;
		case Image::FORMAT_RGBA_ASTC_8x8:
			result = astcenc_config_init(profile, 8, 8,
					1, preset, 0, &config);
			break;
		case Image::FORMAT_RGBA_ASTC_10x5:
			result = astcenc_config_init(profile, 10, 5,
					1, preset, 0, &config);
			break;
		case Image::FORMAT_RGBA_ASTC_10x6:
			result = astcenc_config_init(profile, 10, 6,
					1, preset, 0, &config);
			break;
		case Image::FORMAT_RGBA_ASTC_10x8:
			result = astcenc_config_init(profile, 10, 8,
					1, preset, 0, &config);
			break;
		case Image::FORMAT_RGBA_ASTC_10x10:
			result = astcenc_config_init(profile, 10, 10,
					1, preset, 0, &config);
			break;
		case Image::FORMAT_RGBA_ASTC_12x10:
			result = astcenc_config_init(profile, 12, 10,
					1, preset, 0, &config);
			break;
		case Image::FORMAT_RGBA_ASTC_12x12:
			result = astcenc_config_init(profile, 12, 12,
					1, preset, 0, &config);
			break;
		case Image::FORMAT_SRGB8_ALPHA8_ASTC_4x4:
			result = astcenc_config_init(profile, 4, 4,
					1, preset, 0, &config);
			profile = ASTCENC_PRF_LDR_SRGB;
			break;
		case Image::FORMAT_SRGB8_ALPHA8_ASTC_5x4:
			result = astcenc_config_init(profile, 5, 4,
					1, preset, 0, &config);
			profile = ASTCENC_PRF_LDR_SRGB;
			break;
		case Image::FORMAT_SRGB8_ALPHA8_ASTC_5x5:
			result = astcenc_config_init(profile, 5, 5,
					1, preset, 0, &config);
			profile = ASTCENC_PRF_LDR_SRGB;
			break;
		case Image::FORMAT_SRGB8_ALPHA8_ASTC_6x5:
			result = astcenc_config_init(profile, 6, 5,
					1, preset, 0, &config);
			profile = ASTCENC_PRF_LDR_SRGB;
			break;
		case Image::FORMAT_SRGB8_ALPHA8_ASTC_6x6:
			result = astcenc_config_init(profile, 6, 6,
					1, preset, 0, &config);
			profile = ASTCENC_PRF_LDR_SRGB;
			break;
		case Image::FORMAT_SRGB8_ALPHA8_ASTC_8x5:
			result = astcenc_config_init(profile, 8, 5,
					1, preset, 0, &config);
			profile = ASTCENC_PRF_LDR_SRGB;
			break;
		case Image::FORMAT_SRGB8_ALPHA8_ASTC_8x6:
			result = astcenc_config_init(profile, 8, 6,
					1, preset, 0, &config);
			profile = ASTCENC_PRF_LDR_SRGB;
			break;
		case Image::FORMAT_SRGB8_ALPHA8_ASTC_8x8:
			result = astcenc_config_init(profile, 8, 8,
					1, preset, 0, &config);
			profile = ASTCENC_PRF_LDR_SRGB;
			break;
		case Image::FORMAT_SRGB8_ALPHA8_ASTC_10x5:
			result = astcenc_config_init(profile, 10, 5,
					1, preset, 0, &config);
			profile = ASTCENC_PRF_LDR_SRGB;
			break;
		case Image::FORMAT_SRGB8_ALPHA8_ASTC_10x6:
			result = astcenc_config_init(profile, 10, 6,
					1, preset, 0, &config);
			profile = ASTCENC_PRF_LDR_SRGB;
			break;
		case Image::FORMAT_SRGB8_ALPHA8_ASTC_10x8:
			result = astcenc_config_init(profile, 10, 8,
					1, preset, 0, &config);
			profile = ASTCENC_PRF_LDR_SRGB;
			break;
		case Image::FORMAT_SRGB8_ALPHA8_ASTC_10x10:
			result = astcenc_config_init(profile, 10, 10,
					1, preset, 0, &config);
			profile = ASTCENC_PRF_LDR_SRGB;
			break;
		case Image::FORMAT_SRGB8_ALPHA8_ASTC_12x10:
			result = astcenc_config_init(profile, 12, 10,
					1, preset, 0, &config);
			profile = ASTCENC_PRF_LDR_SRGB;
			break;
		case Image::FORMAT_SRGB8_ALPHA8_ASTC_12x12:
			result = astcenc_config_init(profile, 12, 12,
					1, preset, 0, &config);
			profile = ASTCENC_PRF_LDR_SRGB;
			break;
		default:
			return;
	}
	astcenc_context *codec_context;
	result = astcenc_context_alloc(&config, 1, &codec_context);
	if (result != ASTCENC_SUCCESS) {
		return;
	}

	astcenc_swizzle swizzle;
	swizzle.a = r_img->astc_a ? ASTCENC_SWZ_A : ASTCENC_SWZ_1;
	swizzle.r = r_img->astc_r ? ASTCENC_SWZ_R : ASTCENC_SWZ_0;
	swizzle.g = r_img->astc_g ? ASTCENC_SWZ_G : ASTCENC_SWZ_0;
	swizzle.b = r_img->astc_b ? ASTCENC_SWZ_B : ASTCENC_SWZ_0;
	size_t buffer_size = Image::get_any_image_data_size(r_img->get_width(), r_img->get_height(), Image::FORMAT_RGBA8, mipmaps);

	PackedByteArray data_buf;
	data_buf.resize(buffer_size);
	uint8_t *data_ptr = &data_buf.write[0];

	const uint8_t *src_read = r_img->get_data().ptr();

	astcenc_image *uncompressed_image = astc_img_from_unorm8x4_array(data_ptr, r_img->get_width(), r_img->get_height(), false);

	result = astcenc_decompress_image(codec_context, src_read, Image::get_any_image_data_size(r_img->get_width(), r_img->get_height(), img_format), uncompressed_image, &swizzle, 0); // 输出的缓冲区的大小
	if (result != ASTCENC_SUCCESS) {
		printf("ERROR: Codec decompress failed: %s\n", astcenc_get_error_string(result));
		return;
	}

	free_image(uncompressed_image);
	astcenc_context_free(codec_context);
	// 下面保存解压缩数据
	r_img->create(r_img->get_width(), r_img->get_height(), false, Image::FORMAT_RGBA8, data_buf);
	if (mipmaps) {
		r_img->generate_mipmaps();
	}
}

void initialize_astc_encoder_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	Image::_image_compress_astc_func = &image_compress_astc_func;
	Image::_image_decompress_astc = &image_decompress_astc;
}

void uninitialize_astc_encoder_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	Image::_image_compress_astc_func = nullptr;
	Image::_image_decompress_astc = nullptr;
}
