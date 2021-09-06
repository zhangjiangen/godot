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

#include "core/os/os.h"
#include "servers/rendering_server.h"
#include "texture_astc.h"
#include "core/io/image.h"


static void image_compress_astc_func(Image * p_image, float l, Image::CompressMode p_mode, Image::UsedChannels p_channel)
{	// Compute the number of ASTC blocks in each dimension
	//unsigned int block_count_x = (image_x + block_x - 1) / block_x;
	//unsigned int block_count_y = (image_y + block_y - 1) / block_y;

	//// ------------------------------------------------------------------------
	//// Initialize the default configuration for the block size and quality
	//astcenc_config config;
	//config.block_x = block_x;
	//config.block_y = block_y;
	//config.profile = profile;

	//astcenc_error status;
	//status = astcenc_config_init(profile, block_x, block_y, block_z, quality, 0, &config);
	//if (status != ASTCENC_SUCCESS)
	//{
	//	printf("ERROR: Codec config init failed: %s\n", astcenc_get_error_string(status));
	//	return ;
	//}

	//// ... power users can customize any config settings after calling
	//// config_init() and before calling context alloc().

	//// ------------------------------------------------------------------------
	//// Create a context based on the configuration
	//astcenc_context* context;
	//status = astcenc_context_alloc(&config, thread_count, &context);
	//if (status != ASTCENC_SUCCESS)
	//{
	//	printf("ERROR: Codec context alloc failed: %s\n", astcenc_get_error_string(status));
	//	return ;
	//}

	//// ------------------------------------------------------------------------
	//// Compress the image
	//astcenc_image image;
	//image.dim_x = image_x;
	//image.dim_y = image_y;
	//image.dim_z = 1;
	//image.data_type = ASTCENC_TYPE_U8;
	//uint8_t* slices = image_data;
	//image.data = reinterpret_cast<void**>(&slices);

	//// Space needed for 16 bytes of output per compressed block
	//size_t comp_len = block_count_x * block_count_y * 16;
	//uint8_t* comp_data = new uint8_t[comp_len];

	//status = astcenc_compress_image(context, &image, &swizzle, comp_data, comp_len, 0);
	//if (status != ASTCENC_SUCCESS)
	//{
	//	printf("ERROR: Codec compress failed: %s\n", astcenc_get_error_string(status));
	//	return ;
	//}
	
}
static void image_decompress_astc(Image *)
{
	
}

void register_astc_encoder_types() {
	Image::_image_compress_astc_func = &image_compress_astc_func;
	Image::_image_decompress_astc = &image_decompress_astc;
}

void unregister_astc_encoder_types() {
	Image::_image_compress_astc_func = nullptr;
	Image::_image_decompress_astc = nullptr;
}
