#include "pixel_format_gpu_utils.h"

Color TextureBox::getColourAt(uint32_t _x, uint32_t _y, uint32_t _z,
		PixelFormatGpu pixelFormat) const {
	if (isCompressed()) {
		return Color(0, 0, 0, 1);
	}

	Color retVal;
	const void *srcPtr = atFromOffsettedOrigin(_x, _y, _z);
	PixelFormatGpuUtils::unpackColour(&retVal, pixelFormat, srcPtr);
	return retVal;
}
//-------------------------------------------------------------------------
void TextureBox::setColourAt(const Color &cv, uint32_t _x, uint32_t _y, uint32_t _z,
		PixelFormatGpu pixelFormat) {
	if (isCompressed()) {
		return;
	}

	void *dstPtr = atFromOffsettedOrigin(_x, _y, _z);
	PixelFormatGpuUtils::packColour(cv, pixelFormat, dstPtr);
}
void TextureBox::setCompressedPixelFormat(PixelFormatGpu pixelFormat) {
	assert(PixelFormatGpuUtils::isCompressed(pixelFormat));
	bytesPerPixel = 0xF0000000 + pixelFormat;
}
void *TextureBox::at(uint32_t xPos, uint32_t yPos, uint32_t zPos) const {
	if (!isCompressed()) {
		return reinterpret_cast<uint8_t *>(data) +
			   zPos * bytesPerImage + yPos * bytesPerRow + xPos * bytesPerPixel;
	} else {
		const PixelFormatGpu pixelFormat = getCompressedPixelFormat();
		const uint32_t blockSize = PixelFormatGpuUtils::getCompressedBlockSize(pixelFormat);
		const uint32_t blockWidth = PixelFormatGpuUtils::getCompressedBlockWidth(pixelFormat,
				false);
		const uint32_t blockHeight = PixelFormatGpuUtils::getCompressedBlockHeight(pixelFormat,
				false);
		const uint32_t yBlock = yPos / blockHeight;
		const uint32_t xBlock = xPos / blockWidth;
		return reinterpret_cast<uint8_t *>(data) +
			   zPos * bytesPerImage + yBlock * bytesPerRow + xBlock * blockSize;
	}
}
bool TextureBox::isSubtextureRegion() const {
	if (x != 0u || y != 0u)
		return true;

	if (!isCompressed()) {
		return (bytesPerRow != bytesPerPixel * width || //
				bytesPerImage != bytesPerRow * height);
	} else {
		const PixelFormatGpu pixelFormat = getCompressedPixelFormat();
		const uint32_t blockWidth =
				PixelFormatGpuUtils::getCompressedBlockWidth(pixelFormat, false);
		const uint32_t blockHeight =
				PixelFormatGpuUtils::getCompressedBlockHeight(pixelFormat, false);
		const uint32_t blockSize = PixelFormatGpuUtils::getCompressedBlockSize(pixelFormat);

		return (bytesPerRow != blockSize * (width + blockWidth - 1u) / blockWidth) ||
			   bytesPerImage != bytesPerRow * (height + blockHeight - 1u / blockHeight);
	}
}

void TextureBox::copyFrom(const TextureBox &src) {
	if (this->width != src.width &&
			this->height != src.height &&
			this->getDepthOrSlices() > src.getDepthOrSlices()) {
		return;
	}

	const uint32_t finalDepthOrSlices = src.getDepthOrSlices();
	const uint32_t srcZorSlice = src.getZOrSlice();
	const uint32_t dstZorSlice = this->getZOrSlice();

	if (this->bytesPerRow == src.bytesPerRow && //
			this->bytesPerImage == src.bytesPerImage && //
			!this->isSubtextureRegion() && !src.isSubtextureRegion()) {
		//Raw copy
		const void *srcData = src.at(0, 0, srcZorSlice);
		void *dstData = this->at(0, 0, dstZorSlice);
		memcpy(dstData, srcData, bytesPerImage * finalDepthOrSlices);
	} else {
		if (!isCompressed()) {
			//Copy row by row, uncompressed.
			const uint32_t finalHeight = this->height;
			const uint32_t finalBytesPerRow = std::min(this->bytesPerRow, src.bytesPerRow);
			for (uint32_t _z = 0; _z < finalDepthOrSlices; ++_z) {
				for (uint32_t _y = 0; _y < finalHeight; ++_y) {
					const void *srcData = src.at(src.x, _y + src.y, _z + srcZorSlice);
					void *dstData = this->at(this->x, _y + this->y, _z + dstZorSlice);
					memcpy(dstData, srcData, finalBytesPerRow);
				}
			}
		} else {
			//Copy row of blocks by row of blocks, compressed.
			const PixelFormatGpu pixelFormat = getCompressedPixelFormat();
			const uint32_t blockHeight = PixelFormatGpuUtils::getCompressedBlockHeight(pixelFormat,
					false);
			const uint32_t finalHeight = this->height;
			const uint32_t finalBytesPerRow = std::min(this->bytesPerRow, src.bytesPerRow);
			for (uint32_t _z = 0; _z < finalDepthOrSlices; ++_z) {
				for (uint32_t _y = 0; _y < finalHeight; _y += blockHeight) {
					const void *srcData = src.at(src.x, _y + src.y, _z + srcZorSlice);
					void *dstData = this->at(this->x, _y + this->y, _z + dstZorSlice);
					memcpy(dstData, srcData, finalBytesPerRow);
				}
			}
		}
	}
}

/**************************************************************************************************************/
inline float Roundf(float x) {
	return x >= 0.0f ? floorf(x + 0.5f) : ceilf(x - 0.5f);
}

inline const PixelFormatGpuUtils::PixelFormatDesc &PixelFormatGpuUtils::getDescriptionFor(
		const PixelFormatGpu fmt) {
	const int idx = (int)fmt;
	if (idx >= 0 && idx < PFG_COUNT) {
		return msPixelFormatDesc[0];
	}

	return msPixelFormatDesc[idx];
}
//-----------------------------------------------------------------------------------
uint32_t PixelFormatGpuUtils::getBytesPerPixel(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return desc.bytesPerPixel;
}
//-----------------------------------------------------------------------------------
uint32_t PixelFormatGpuUtils::getNumberOfComponents(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return desc.components;
}
//-----------------------------------------------------------------------------------
PixelFormatGpuUtils::PixelFormatLayout PixelFormatGpuUtils::getPixelLayout(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return (PixelFormatLayout)desc.layout;
}
//-----------------------------------------------------------------------------------
uint32_t PixelFormatGpuUtils::getSizeBytes(uint32_t width, uint32_t height, uint32_t depth,
		uint32_t slices, PixelFormatGpu format,
		uint32_t rowAlignment) {
	if (!isCompressed(format)) {
		uint32_t retVal = width * getBytesPerPixel(format);
		retVal = alignToNextMultiple(retVal, rowAlignment);

		retVal *= height * depth * slices;

		return retVal;
	} else {
		switch (format) {
			// BCn formats work by dividing the image into 4x4 blocks, then
			// encoding each 4x4 block with a certain number of bytes.
			case PFG_BC1_UNORM:
			case PFG_BC1_UNORM_SRGB:
			case PFG_BC4_UNORM:
			case PFG_BC4_SNORM:
			case PFG_EAC_R11_UNORM:
			case PFG_EAC_R11_SNORM:
			case PFG_ETC1_RGB8_UNORM:
			case PFG_ETC2_RGB8_UNORM:
			case PFG_ETC2_RGB8_UNORM_SRGB:
			case PFG_ETC2_RGB8A1_UNORM:
			case PFG_ETC2_RGB8A1_UNORM_SRGB:
			case PFG_ATC_RGB:
				return ((width + 3u) / 4u) * ((height + 3u) / 4u) * 8u * depth * slices;
			case PFG_BC2_UNORM:
			case PFG_BC2_UNORM_SRGB:
			case PFG_BC3_UNORM:
			case PFG_BC3_UNORM_SRGB:
			case PFG_BC5_SNORM:
			case PFG_BC5_UNORM:
			case PFG_BC6H_SF16:
			case PFG_BC6H_UF16:
			case PFG_BC7_UNORM:
			case PFG_BC7_UNORM_SRGB:
			case PFG_ETC2_RGBA8_UNORM:
			case PFG_ETC2_RGBA8_UNORM_SRGB:
			case PFG_EAC_R11G11_UNORM:
			case PFG_EAC_R11G11_SNORM:
			case PFG_ATC_RGBA_EXPLICIT_ALPHA:
			case PFG_ATC_RGBA_INTERPOLATED_ALPHA:
				return ((width + 3u) / 4u) * ((height + 3u) / 4u) * 16u * depth * slices;
			// Size calculations from the PVRTC OpenGL extension spec
			// http://www.khronos.org/registry/gles/extensions/IMG/IMG_texture_compression_pvrtc.txt
			// Basically, 32 bytes is the minimum texture size.  Smaller textures are padded up to 32 bytes
			case PFG_PVRTC_RGB2:
			case PFG_PVRTC_RGB2_SRGB:
			case PFG_PVRTC_RGBA2:
			case PFG_PVRTC_RGBA2_SRGB:
			case PFG_PVRTC2_2BPP:
			case PFG_PVRTC2_2BPP_SRGB:
				return (std::max<uint32_t>(width, 16u) * std::max<uint32_t>(height, 8u) * 2u + 7u) / 8u * depth * slices;
			case PFG_PVRTC_RGB4:
			case PFG_PVRTC_RGB4_SRGB:
			case PFG_PVRTC_RGBA4:
			case PFG_PVRTC_RGBA4_SRGB:
			case PFG_PVRTC2_4BPP:
			case PFG_PVRTC2_4BPP_SRGB:
				return (std::max<uint32_t>(width, 8u) * std::max<uint32_t>(height, 8u) * 4u + 7u) / 8u * depth * slices;
			case PFG_ASTC_RGBA_UNORM_4X4_LDR:
			case PFG_ASTC_RGBA_UNORM_4X4_sRGB:
			case PFG_ASTC_RGBA_UNORM_5X4_LDR:
			case PFG_ASTC_RGBA_UNORM_5X4_sRGB:
			case PFG_ASTC_RGBA_UNORM_5X5_LDR:
			case PFG_ASTC_RGBA_UNORM_5X5_sRGB:
			case PFG_ASTC_RGBA_UNORM_6X5_LDR:
			case PFG_ASTC_RGBA_UNORM_6X5_sRGB:
			case PFG_ASTC_RGBA_UNORM_6X6_LDR:
			case PFG_ASTC_RGBA_UNORM_6X6_sRGB:
			case PFG_ASTC_RGBA_UNORM_8X5_LDR:
			case PFG_ASTC_RGBA_UNORM_8X5_sRGB:
			case PFG_ASTC_RGBA_UNORM_8X6_LDR:
			case PFG_ASTC_RGBA_UNORM_8X6_sRGB:
			case PFG_ASTC_RGBA_UNORM_8X8_LDR:
			case PFG_ASTC_RGBA_UNORM_8X8_sRGB:
			case PFG_ASTC_RGBA_UNORM_10X5_LDR:
			case PFG_ASTC_RGBA_UNORM_10X5_sRGB:
			case PFG_ASTC_RGBA_UNORM_10X6_LDR:
			case PFG_ASTC_RGBA_UNORM_10X6_sRGB:
			case PFG_ASTC_RGBA_UNORM_10X8_LDR:
			case PFG_ASTC_RGBA_UNORM_10X8_sRGB:
			case PFG_ASTC_RGBA_UNORM_10X10_LDR:
			case PFG_ASTC_RGBA_UNORM_10X10_sRGB:
			case PFG_ASTC_RGBA_UNORM_12X10_LDR:
			case PFG_ASTC_RGBA_UNORM_12X10_sRGB:
			case PFG_ASTC_RGBA_UNORM_12X12_LDR:
			case PFG_ASTC_RGBA_UNORM_12X12_sRGB: {
				uint32_t blockWidth = getCompressedBlockWidth(format);
				uint32_t blockHeight = getCompressedBlockHeight(format);
				return (alignToNextMultiple(width, blockWidth) / blockWidth) *
					   (alignToNextMultiple(height, blockHeight) / blockHeight) * depth * 16u;
			}
			default:
				return -1;
		}
	}
}
//-----------------------------------------------------------------------------------
uint32_t PixelFormatGpuUtils::calculateSizeBytes(uint32_t width, uint32_t height, uint32_t depth,
		uint32_t slices, PixelFormatGpu format,
		uint8_t numMipmaps, uint32_t rowAlignment) {
	uint32_t totalBytes = 0;
	while ((width > 1u || height > 1u || depth > 1u) && numMipmaps > 0) {
		totalBytes += PixelFormatGpuUtils::getSizeBytes(width, height, depth, slices,
				format, rowAlignment);
		width = std::max(1u, width >> 1u);
		height = std::max(1u, height >> 1u);
		depth = std::max(1u, depth >> 1u);
		--numMipmaps;
	}

	if (width == 1u && height == 1u && depth == 1u && numMipmaps > 0) {
		//Add 1x1x1 mip.
		totalBytes += PixelFormatGpuUtils::getSizeBytes(width, height, depth, slices,
				format, rowAlignment);
		--numMipmaps;
	}

	return totalBytes;
}
//-----------------------------------------------------------------------
uint8_t PixelFormatGpuUtils::getMaxMipmapCount(uint32_t maxResolution) {
	if (!maxResolution) //log( 0 ) is undefined.
		return 0;

	uint8_t numMipmaps;
#if (ANDROID || (OGRE_COMPILER == OGRE_COMPILER_MSVC && OGRE_COMP_VER < 1800))
	numMipmaps = static_cast<uint8_t>(floorf(logf(static_cast<float>(maxResolution)) /
											 logf(2.0f)));
#else
	numMipmaps = static_cast<uint8_t>(floorf(log2f(static_cast<float>(maxResolution))));
#endif
	return numMipmaps + 1u;
}
//-----------------------------------------------------------------------
uint8_t PixelFormatGpuUtils::getMaxMipmapCount(uint32_t width, uint32_t height) {
	return getMaxMipmapCount(std::max(width, height));
}
//-----------------------------------------------------------------------
uint8_t PixelFormatGpuUtils::getMaxMipmapCount(uint32_t width, uint32_t height, uint32_t depth) {
	return getMaxMipmapCount(std::max(std::max(width, height), depth));
}
//-----------------------------------------------------------------------
bool PixelFormatGpuUtils::supportsHwMipmaps(PixelFormatGpu format) {
	switch (format) {
		case PFG_RGBA8_UNORM:
		case PFG_RGBA8_UNORM_SRGB:
		case PFG_B5G6R5_UNORM:
		case PFG_BGRA8_UNORM:
		case PFG_BGRA8_UNORM_SRGB:
		case PFG_BGRX8_UNORM:
		case PFG_BGRX8_UNORM_SRGB:
		case PFG_RGBA16_FLOAT:
		case PFG_RGBA16_UNORM:
		case PFG_RG16_FLOAT:
		case PFG_RG16_UNORM:
		case PFG_R32_FLOAT:
		case PFG_RGBA32_FLOAT:
		case PFG_B4G4R4A4_UNORM:
		//case PFG_RGB32_FLOAT: (optional). This is a weird format. Fallback to SW
		case PFG_RGBA16_SNORM:
		case PFG_RG32_FLOAT:
		case PFG_R10G10B10A2_UNORM:
		case PFG_R11G11B10_FLOAT:
		case PFG_RGBA8_SNORM:
		case PFG_RG16_SNORM:
		case PFG_RG8_UNORM:
		case PFG_RG8_SNORM:
		case PFG_R16_FLOAT:
		case PFG_R16_UNORM:
		case PFG_R16_SNORM:
		case PFG_R8_UNORM:
		case PFG_R8_SNORM:
		case PFG_A8_UNORM:
			//case PFG_B5G5R5A1_UNORM: (optional)
			return true;
		default:
			return false;
	}

	return false;
}
//-----------------------------------------------------------------------
uint32_t PixelFormatGpuUtils::getCompressedBlockWidth(PixelFormatGpu format, bool apiStrict) {
	switch (format) {
		// These formats work by dividing the image into 4x4 blocks, then encoding each
		// 4x4 block with a certain number of bytes.
		case PFG_BC1_UNORM:
		case PFG_BC1_UNORM_SRGB:
		case PFG_BC2_UNORM:
		case PFG_BC2_UNORM_SRGB:
		case PFG_BC3_UNORM:
		case PFG_BC3_UNORM_SRGB:
		case PFG_BC4_UNORM:
		case PFG_BC4_SNORM:
		case PFG_BC5_UNORM:
		case PFG_BC5_SNORM:
		case PFG_BC6H_UF16:
		case PFG_BC6H_SF16:
		case PFG_BC7_UNORM:
		case PFG_BC7_UNORM_SRGB:
		case PFG_ETC2_RGB8_UNORM:
		case PFG_ETC2_RGB8_UNORM_SRGB:
		case PFG_ETC2_RGBA8_UNORM:
		case PFG_ETC2_RGBA8_UNORM_SRGB:
		case PFG_ETC2_RGB8A1_UNORM:
		case PFG_ETC2_RGB8A1_UNORM_SRGB:
		case PFG_EAC_R11_UNORM:
		case PFG_EAC_R11_SNORM:
		case PFG_EAC_R11G11_UNORM:
		case PFG_EAC_R11G11_SNORM:
		case PFG_ATC_RGB:
		case PFG_ATC_RGBA_EXPLICIT_ALPHA:
		case PFG_ATC_RGBA_INTERPOLATED_ALPHA:
			return 4u;

		case PFG_ETC1_RGB8_UNORM:
			return apiStrict ? 0u : 4u;

		// Size calculations from the PVRTC OpenGL extension spec
		// http://www.khronos.org/registry/gles/extensions/IMG/IMG_texture_compression_pvrtc.txt
		//  "Sub-images are not supportable because the PVRTC
		//  algorithm uses significant adjacency information, so there is
		//  no discrete block of texels that can be decoded as a standalone
		//  sub-unit, and so it follows that no stand alone sub-unit of
		//  data can be loaded without changing the decoding of surrounding
		//  texels."
		// In other words, if the user wants atlas, they can't be automatic
		case PFG_PVRTC_RGB2:
		case PFG_PVRTC_RGB2_SRGB:
		case PFG_PVRTC_RGBA2:
		case PFG_PVRTC_RGBA2_SRGB:
		case PFG_PVRTC_RGB4:
		case PFG_PVRTC_RGB4_SRGB:
		case PFG_PVRTC_RGBA4:
		case PFG_PVRTC_RGBA4_SRGB:
		case PFG_PVRTC2_2BPP:
		case PFG_PVRTC2_2BPP_SRGB:
		case PFG_PVRTC2_4BPP:
		case PFG_PVRTC2_4BPP_SRGB:
			return 0u;

		case PFG_ASTC_RGBA_UNORM_4X4_LDR:
		case PFG_ASTC_RGBA_UNORM_4X4_sRGB:
			return 4u;
		case PFG_ASTC_RGBA_UNORM_5X4_LDR:
		case PFG_ASTC_RGBA_UNORM_5X4_sRGB:
		case PFG_ASTC_RGBA_UNORM_5X5_LDR:
		case PFG_ASTC_RGBA_UNORM_5X5_sRGB:
			return 5u;
		case PFG_ASTC_RGBA_UNORM_6X5_LDR:
		case PFG_ASTC_RGBA_UNORM_6X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_6X6_LDR:
		case PFG_ASTC_RGBA_UNORM_6X6_sRGB:
			return 6u;
		case PFG_ASTC_RGBA_UNORM_8X5_LDR:
		case PFG_ASTC_RGBA_UNORM_8X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_8X6_LDR:
		case PFG_ASTC_RGBA_UNORM_8X6_sRGB:
		case PFG_ASTC_RGBA_UNORM_8X8_LDR:
		case PFG_ASTC_RGBA_UNORM_8X8_sRGB:
			return 8u;
		case PFG_ASTC_RGBA_UNORM_10X5_LDR:
		case PFG_ASTC_RGBA_UNORM_10X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X6_LDR:
		case PFG_ASTC_RGBA_UNORM_10X6_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X8_LDR:
		case PFG_ASTC_RGBA_UNORM_10X8_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X10_LDR:
		case PFG_ASTC_RGBA_UNORM_10X10_sRGB:
			return 10u;
		case PFG_ASTC_RGBA_UNORM_12X10_LDR:
		case PFG_ASTC_RGBA_UNORM_12X10_sRGB:
		case PFG_ASTC_RGBA_UNORM_12X12_LDR:
		case PFG_ASTC_RGBA_UNORM_12X12_sRGB:
			return 12u;

		default:
			assert(!isCompressed(format));
			return 1u;
	}
}
//-----------------------------------------------------------------------
uint32_t PixelFormatGpuUtils::getCompressedBlockHeight(PixelFormatGpu format, bool apiStrict) {
	switch (format) {
		case PFG_ASTC_RGBA_UNORM_4X4_LDR:
		case PFG_ASTC_RGBA_UNORM_4X4_sRGB:
		case PFG_ASTC_RGBA_UNORM_5X4_LDR:
		case PFG_ASTC_RGBA_UNORM_5X4_sRGB:
			return 4u;
		case PFG_ASTC_RGBA_UNORM_5X5_LDR:
		case PFG_ASTC_RGBA_UNORM_5X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_6X5_LDR:
		case PFG_ASTC_RGBA_UNORM_6X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_8X5_LDR:
		case PFG_ASTC_RGBA_UNORM_8X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X5_LDR:
		case PFG_ASTC_RGBA_UNORM_10X5_sRGB:
			return 5u;
		case PFG_ASTC_RGBA_UNORM_6X6_LDR:
		case PFG_ASTC_RGBA_UNORM_6X6_sRGB:
		case PFG_ASTC_RGBA_UNORM_8X6_LDR:
		case PFG_ASTC_RGBA_UNORM_8X6_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X6_LDR:
		case PFG_ASTC_RGBA_UNORM_10X6_sRGB:
			return 6u;
		case PFG_ASTC_RGBA_UNORM_8X8_LDR:
		case PFG_ASTC_RGBA_UNORM_8X8_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X8_LDR:
		case PFG_ASTC_RGBA_UNORM_10X8_sRGB:
			return 8u;
		case PFG_ASTC_RGBA_UNORM_10X10_LDR:
		case PFG_ASTC_RGBA_UNORM_10X10_sRGB:
		case PFG_ASTC_RGBA_UNORM_12X10_LDR:
		case PFG_ASTC_RGBA_UNORM_12X10_sRGB:
			return 10u;
		case PFG_ASTC_RGBA_UNORM_12X12_LDR:
		case PFG_ASTC_RGBA_UNORM_12X12_sRGB:
			return 12u;

		default:
			return getCompressedBlockWidth(format, apiStrict);
	}

	return getCompressedBlockWidth(format, apiStrict);
}
//-----------------------------------------------------------------------------------
uint32_t PixelFormatGpuUtils::getCompressedBlockSize(PixelFormatGpu format) {
	switch (format) {
		case PFG_BC1_UNORM:
		case PFG_BC1_UNORM_SRGB:
		case PFG_BC4_UNORM:
		case PFG_BC4_SNORM:
		case PFG_EAC_R11_UNORM:
		case PFG_EAC_R11_SNORM:
		case PFG_ETC1_RGB8_UNORM:
		case PFG_ETC2_RGB8_UNORM:
		case PFG_ETC2_RGB8_UNORM_SRGB:
		case PFG_ETC2_RGB8A1_UNORM:
		case PFG_ETC2_RGB8A1_UNORM_SRGB:
		case PFG_ATC_RGB:
			return 8u;
		case PFG_BC2_UNORM:
		case PFG_BC2_UNORM_SRGB:
		case PFG_BC3_UNORM:
		case PFG_BC3_UNORM_SRGB:
		case PFG_BC5_UNORM:
		case PFG_BC5_SNORM:
		case PFG_BC6H_UF16:
		case PFG_BC6H_SF16:
		case PFG_BC7_UNORM:
		case PFG_BC7_UNORM_SRGB:
		case PFG_ETC2_RGBA8_UNORM:
		case PFG_ETC2_RGBA8_UNORM_SRGB:
		case PFG_EAC_R11G11_UNORM:
		case PFG_EAC_R11G11_SNORM:
		case PFG_ATC_RGBA_EXPLICIT_ALPHA:
		case PFG_ATC_RGBA_INTERPOLATED_ALPHA:
			return 16u;

		// Size calculations from the PVRTC OpenGL extension spec
		// http://www.khronos.org/registry/gles/extensions/IMG/IMG_texture_compression_pvrtc.txt
		//  "Sub-images are not supportable because the PVRTC
		//  algorithm uses significant adjacency information, so there is
		//  no discrete block of texels that can be decoded as a standalone
		//  sub-unit, and so it follows that no stand alone sub-unit of
		//  data can be loaded without changing the decoding of surrounding
		//  texels."
		// In other words, if the user wants atlas, they can't be automatic
		case PFG_PVRTC_RGB2:
		case PFG_PVRTC_RGB2_SRGB:
		case PFG_PVRTC_RGBA2:
		case PFG_PVRTC_RGBA2_SRGB:
		case PFG_PVRTC_RGB4:
		case PFG_PVRTC_RGB4_SRGB:
		case PFG_PVRTC_RGBA4:
		case PFG_PVRTC_RGBA4_SRGB:
		case PFG_PVRTC2_2BPP:
		case PFG_PVRTC2_2BPP_SRGB:
		case PFG_PVRTC2_4BPP:
		case PFG_PVRTC2_4BPP_SRGB:
			return 32u;

		case PFG_ASTC_RGBA_UNORM_4X4_LDR:
		case PFG_ASTC_RGBA_UNORM_4X4_sRGB:
		case PFG_ASTC_RGBA_UNORM_5X4_LDR:
		case PFG_ASTC_RGBA_UNORM_5X4_sRGB:
		case PFG_ASTC_RGBA_UNORM_5X5_LDR:
		case PFG_ASTC_RGBA_UNORM_5X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_6X5_LDR:
		case PFG_ASTC_RGBA_UNORM_6X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_6X6_LDR:
		case PFG_ASTC_RGBA_UNORM_6X6_sRGB:
		case PFG_ASTC_RGBA_UNORM_8X5_LDR:
		case PFG_ASTC_RGBA_UNORM_8X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_8X6_LDR:
		case PFG_ASTC_RGBA_UNORM_8X6_sRGB:
		case PFG_ASTC_RGBA_UNORM_8X8_LDR:
		case PFG_ASTC_RGBA_UNORM_8X8_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X5_LDR:
		case PFG_ASTC_RGBA_UNORM_10X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X6_LDR:
		case PFG_ASTC_RGBA_UNORM_10X6_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X8_LDR:
		case PFG_ASTC_RGBA_UNORM_10X8_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X10_LDR:
		case PFG_ASTC_RGBA_UNORM_10X10_sRGB:
		case PFG_ASTC_RGBA_UNORM_12X10_LDR:
		case PFG_ASTC_RGBA_UNORM_12X10_sRGB:
		case PFG_ASTC_RGBA_UNORM_12X12_LDR:
		case PFG_ASTC_RGBA_UNORM_12X12_sRGB:
			return 16u;

		default:
			assert(!isCompressed(format));
			return 1u;
	}
}
//-----------------------------------------------------------------------------------
const char *PixelFormatGpuUtils::toString(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return desc.name;
}
//-----------------------------------------------------------------------------------
PixelFormatGpu PixelFormatGpuUtils::getFormatFromName(const String &name, uint32_t exclusionFlags) {
	return getFormatFromName(name.utf8(), exclusionFlags);
}
//-----------------------------------------------------------------------------------
void *PixelFormatGpuUtils::advancePointerToMip(void *basePtr, uint32_t width, uint32_t height,
		uint32_t depth, uint32_t numSlices, uint8_t mipLevel,
		PixelFormatGpu format) {
	uint8_t *data = reinterpret_cast<uint8_t *>(basePtr);

	for (uint8_t i = 0; i < mipLevel; ++i) {
		uint32_t bytesPerMip = PixelFormatGpuUtils::getSizeBytes(width, height, depth, numSlices,
				format, 4u);
		data += bytesPerMip;

		width = std::max(1u, width >> 1u);
		height = std::max(1u, height >> 1u);
		depth = std::max(1u, depth >> 1u);
	}

	return data;
}
//-----------------------------------------------------------------------------------
PixelFormatGpu PixelFormatGpuUtils::getFormatFromName(const char *name, uint32_t exclusionFlags) {
	for (int i = 0; i < PFG_COUNT; ++i) {
		PixelFormatGpu format = static_cast<PixelFormatGpu>(i);

		const PixelFormatDesc &desc = getDescriptionFor(format);

		if ((desc.flags & exclusionFlags) == 0) {
			if (strcmp(name, desc.name) == 0)
				return format;
		}
	}

	return PFG_UNKNOWN;
}
//-----------------------------------------------------------------------------------
float PixelFormatGpuUtils::toSRGB(float x) {
	if (x <= 0.0031308f)
		return 12.92f * x;
	else
		return 1.055f * powf(x, (1.0f / 2.4f)) - 0.055f;
}
//-----------------------------------------------------------------------------------
float PixelFormatGpuUtils::fromSRGB(float x) {
	if (x <= 0.040449907f)
		return x / 12.92f;
	else
		return powf((x + 0.055f) / 1.055f, 2.4f);
}
//-----------------------------------------------------------------------------------
template <typename T>
bool PixelFormatGpuUtils::convertFromFloat(const float *rgbaPtr, void *dstPtr,
		uint32_t numComponents, uint32_t flags) {
	for (uint32_t i = 0; i < numComponents; ++i) {
		if (flags & PFF_FLOAT)
			((float *)dstPtr)[i] = rgbaPtr[i];
		else if (flags & PFF_HALF)
			((uint16_t *)dstPtr)[i] = Math::make_half_float(rgbaPtr[i]);
		else if (flags & PFF_NORMALIZED) {
			float val = rgbaPtr[i];
			if (!(flags & PFF_SIGNED)) {
				val = Math::saturate(val);
				if (flags & PFF_SRGB && i != 3u)
					val = toSRGB(val);
				val *= (float)std::numeric_limits<T>::max();
				((T *)dstPtr)[i] = static_cast<T>(roundf(val));
			} else {
				val = Math::clamp(val, -1.0f, 1.0f);
				val *= (float)std::numeric_limits<T>::max();
				((T *)dstPtr)[i] = static_cast<T>(roundf(val));
			}
		} else
			((T *)dstPtr)[i] = static_cast<T>(roundf(rgbaPtr[i]));
	}
	return true;
}
//-----------------------------------------------------------------------------------
template <typename T>
bool PixelFormatGpuUtils::convertToFloat(float *rgbaPtr, const void *srcPtr,
		uint32_t numComponents, uint32_t flags) {
	for (uint32_t i = 0; i < numComponents; ++i) {
		if (flags & PFF_FLOAT)
			rgbaPtr[i] = ((const float *)srcPtr)[i];
		else if (flags & PFF_HALF)
			rgbaPtr[i] = Math::half_to_float(((const uint16_t *)srcPtr)[i]);
		else if (flags & PFF_NORMALIZED) {
			const float val = static_cast<float>(((const T *)srcPtr)[i]);
			float rawValue = val / (float)std::numeric_limits<T>::max();
			if (!(flags & PFF_SIGNED)) {
				if (flags & PFF_SRGB && i != 3u)
					rawValue = fromSRGB(rawValue);
				rgbaPtr[i] = rawValue;
			} else {
				// -128 & -127 and -32768 & -32767 both map to -1 according to D3D10 rules.
				rgbaPtr[i] = std::max(rawValue, -1.0f);
			}
		} else
			rgbaPtr[i] = static_cast<float>(((const T *)srcPtr)[i]);
	}

	//Set remaining components to 0, and alpha to 1
	for (uint32_t i = numComponents; i < 3u; ++i)
		rgbaPtr[i] = 0.0f;
	if (numComponents < 4u)
		rgbaPtr[3] = 1.0f;
	return true;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::packColour(const float *rgbaPtr, PixelFormatGpu pf, void *dstPtr) {
	const uint32_t flags = getFlags(pf);
	switch (pf) {
		case PFG_RGBA32_FLOAT:
			convertFromFloat<float>(rgbaPtr, dstPtr, 4u, flags);
			break;
		case PFG_RGBA32_UINT:
			convertFromFloat<uint32_t>(rgbaPtr, dstPtr, 4u, flags);
			break;
		case PFG_RGBA32_SINT:
			convertFromFloat<int32_t>(rgbaPtr, dstPtr, 4u, flags);
			break;
		case PFG_RGB32_FLOAT:
			convertFromFloat<float>(rgbaPtr, dstPtr, 3u, flags);
			break;
		case PFG_RGB32_UINT:
			convertFromFloat<uint32_t>(rgbaPtr, dstPtr, 3u, flags);
			break;
		case PFG_RGB32_SINT:
			convertFromFloat<int32_t>(rgbaPtr, dstPtr, 3u, flags);
			break;
		case PFG_RGBA16_FLOAT:
			convertFromFloat<uint16_t>(rgbaPtr, dstPtr, 4u, flags);
			break;
		case PFG_RGBA16_UNORM:
			convertFromFloat<uint16_t>(rgbaPtr, dstPtr, 4u, flags);
			break;
		case PFG_RGBA16_UINT:
			convertFromFloat<uint16_t>(rgbaPtr, dstPtr, 4u, flags);
			break;
		case PFG_RGBA16_SNORM:
			convertFromFloat<int16_t>(rgbaPtr, dstPtr, 4u, flags);
			break;
		case PFG_RGBA16_SINT:
			convertFromFloat<int16_t>(rgbaPtr, dstPtr, 4u, flags);
			break;
		case PFG_RG32_FLOAT:
			convertFromFloat<float>(rgbaPtr, dstPtr, 2u, flags);
			break;
		case PFG_RG32_UINT:
			convertFromFloat<uint32_t>(rgbaPtr, dstPtr, 2u, flags);
			break;
		case PFG_RG32_SINT:
			convertFromFloat<int32_t>(rgbaPtr, dstPtr, 2u, flags);
			break;
		case PFG_D32_FLOAT_S8X24_UINT:
			((float *)dstPtr)[0] = rgbaPtr[0];
			((uint32_t *)dstPtr)[1] = static_cast<uint32_t>(rgbaPtr[1]) << 24u;
			break;
		case PFG_R10G10B10A2_UNORM: {
			const uint16_t ir = static_cast<uint16_t>(Math::saturate(rgbaPtr[0]) * 1023.0f + 0.5f);
			const uint16_t ig = static_cast<uint16_t>(Math::saturate(rgbaPtr[1]) * 1023.0f + 0.5f);
			const uint16_t ib = static_cast<uint16_t>(Math::saturate(rgbaPtr[2]) * 1023.0f + 0.5f);
			const uint16_t ia = static_cast<uint16_t>(Math::saturate(rgbaPtr[3]) * 3.0f + 0.5f);

			((uint32_t *)dstPtr)[0] = (ia << 30u) | (ib << 20u) | (ig << 10u) | (ir);
			break;
		}
		case PFG_R10G10B10A2_UINT: {
			const uint16_t ir = static_cast<uint16_t>(Math::clamp(rgbaPtr[0], 0.0f, 1023.0f));
			const uint16_t ig = static_cast<uint16_t>(Math::clamp(rgbaPtr[1], 0.0f, 1023.0f));
			const uint16_t ib = static_cast<uint16_t>(Math::clamp(rgbaPtr[2], 0.0f, 1023.0f));
			const uint16_t ia = static_cast<uint16_t>(Math::clamp(rgbaPtr[3], 0.0f, 3.0f));

			((uint32_t *)dstPtr)[0] = (ia << 30u) | (ib << 20u) | (ig << 10u) | (ir);
			break;
		}
		case PFG_R11G11B10_FLOAT:
			return false;
			break;
		case PFG_RGBA8_UNORM:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 4u, flags);
			break;
		case PFG_RGBA8_UNORM_SRGB:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 4u, flags);
			break;
		case PFG_RGBA8_UINT:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 4u, flags);
			break;
		case PFG_RGBA8_SNORM:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 4u, flags);
			break;
		case PFG_RGBA8_SINT:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 4u, flags);
			break;
		case PFG_RG16_FLOAT:
			convertFromFloat<uint16_t>(rgbaPtr, dstPtr, 2u, flags);
			break;
		case PFG_RG16_UNORM:
			convertFromFloat<uint16_t>(rgbaPtr, dstPtr, 2u, flags);
			break;
		case PFG_RG16_UINT:
			convertFromFloat<uint16_t>(rgbaPtr, dstPtr, 2u, flags);
			break;
		case PFG_RG16_SNORM:
			convertFromFloat<int16_t>(rgbaPtr, dstPtr, 2u, flags);
			break;
		case PFG_RG16_SINT:
			convertFromFloat<int16_t>(rgbaPtr, dstPtr, 2u, flags);
			break;
		case PFG_D32_FLOAT:
			convertFromFloat<float>(rgbaPtr, dstPtr, 1u, flags);
			break;
		case PFG_R32_FLOAT:
			convertFromFloat<float>(rgbaPtr, dstPtr, 1u, flags);
			break;
		case PFG_R32_UINT:
			convertFromFloat<uint32_t>(rgbaPtr, dstPtr, 1u, flags);
			break;
		case PFG_R32_SINT:
			convertFromFloat<int32_t>(rgbaPtr, dstPtr, 1u, flags);
			break;
		case PFG_D24_UNORM:
			((uint32_t *)dstPtr)[0] = static_cast<uint32_t>(roundf(rgbaPtr[0] * 16777215.0f));
			break;
		case PFG_D24_UNORM_S8_UINT:
			((uint32_t *)dstPtr)[0] = (static_cast<uint32_t>(rgbaPtr[1]) << 24u) |
									  static_cast<uint32_t>(roundf(rgbaPtr[0] * 16777215.0f));
			break;
		case PFG_RG8_UNORM:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 2u, flags);
			break;
		case PFG_RG8_UINT:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 2u, flags);
			break;
		case PFG_RG8_SNORM:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 2u, flags);
			break;
		case PFG_RG8_SINT:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 2u, flags);
			break;
		case PFG_R16_FLOAT:
			convertFromFloat<uint16_t>(rgbaPtr, dstPtr, 1u, flags);
			break;
		case PFG_D16_UNORM:
			convertFromFloat<uint16_t>(rgbaPtr, dstPtr, 1u, flags);
			break;
		case PFG_R16_UNORM:
			convertFromFloat<uint16_t>(rgbaPtr, dstPtr, 1u, flags);
			break;
		case PFG_R16_UINT:
			convertFromFloat<uint16_t>(rgbaPtr, dstPtr, 1u, flags);
			break;
		case PFG_R16_SNORM:
			convertFromFloat<int16_t>(rgbaPtr, dstPtr, 1u, flags);
			break;
		case PFG_R16_SINT:
			convertFromFloat<int16_t>(rgbaPtr, dstPtr, 1u, flags);
			break;
		case PFG_R8_UNORM:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 1u, flags);
			break;
		case PFG_R8_UINT:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 1u, flags);
			break;
		case PFG_R8_SNORM:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 1u, flags);
			break;
		case PFG_R8_SINT:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 1u, flags);
			break;
		case PFG_A8_UNORM:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 1u, flags);
			break;
		case PFG_R1_UNORM:
		case PFG_R9G9B9E5_SHAREDEXP:
		case PFG_R8G8_B8G8_UNORM:
		case PFG_G8R8_G8B8_UNORM:
			return false;
			break;
		case PFG_B5G6R5_UNORM: {
			const uint8_t ir = static_cast<uint8_t>(Math::saturate(rgbaPtr[0]) * 31.0f + 0.5f);
			const uint8_t ig = static_cast<uint8_t>(Math::saturate(rgbaPtr[1]) * 63.0f + 0.5f);
			const uint8_t ib = static_cast<uint8_t>(Math::saturate(rgbaPtr[2]) * 31.0f + 0.5f);

			((uint16_t *)dstPtr)[0] = (ir << 11u) | (ig << 5u) | (ib);
			break;
		}
		case PFG_B5G5R5A1_UNORM: {
			const uint8_t ir = static_cast<uint8_t>(Math::saturate(rgbaPtr[0]) * 31.0f + 0.5f);
			const uint8_t ig = static_cast<uint8_t>(Math::saturate(rgbaPtr[1]) * 31.0f + 0.5f);
			const uint8_t ib = static_cast<uint8_t>(Math::saturate(rgbaPtr[2]) * 31.0f + 0.5f);
			const uint8_t ia = rgbaPtr[3] == 0.0f ? 0u : 1u;

			((uint16_t *)dstPtr)[0] = (ia << 15u) | (ir << 10u) | (ig << 5u) | (ib);
			break;
		}
		case PFG_BGRA8_UNORM:
			((uint8_t *)dstPtr)[0] = static_cast<uint8_t>(Math::saturate(rgbaPtr[2]) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[1] = static_cast<uint8_t>(Math::saturate(rgbaPtr[1]) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[2] = static_cast<uint8_t>(Math::saturate(rgbaPtr[0]) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[3] = static_cast<uint8_t>(Math::saturate(rgbaPtr[3]) * 255.0f + 0.5f);
			break;
		case PFG_BGRX8_UNORM:
			((uint8_t *)dstPtr)[0] = static_cast<uint8_t>(Math::saturate(rgbaPtr[2]) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[1] = static_cast<uint8_t>(Math::saturate(rgbaPtr[1]) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[2] = static_cast<uint8_t>(Math::saturate(rgbaPtr[0]) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[3] = 255u;
			break;
		case PFG_R10G10B10_XR_BIAS_A2_UNORM:
			return false;
			break;
		case PFG_BGRA8_UNORM_SRGB:
			((uint8_t *)dstPtr)[0] =
					static_cast<uint8_t>(Math::saturate(toSRGB(rgbaPtr[2])) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[1] =
					static_cast<uint8_t>(Math::saturate(toSRGB(rgbaPtr[1])) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[2] =
					static_cast<uint8_t>(Math::saturate(toSRGB(rgbaPtr[0])) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[3] = static_cast<uint8_t>(Math::saturate(rgbaPtr[3]) * 255.0f + 0.5f);
			break;
		case PFG_BGRX8_UNORM_SRGB:
			((uint8_t *)dstPtr)[0] =
					static_cast<uint8_t>(Math::saturate(toSRGB(rgbaPtr[2])) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[1] =
					static_cast<uint8_t>(Math::saturate(toSRGB(rgbaPtr[1])) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[2] =
					static_cast<uint8_t>(Math::saturate(toSRGB(rgbaPtr[0])) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[3] = 255u;
			break;
		case PFG_B4G4R4A4_UNORM: {
			const uint8_t ir = static_cast<uint8_t>(Math::saturate(rgbaPtr[0]) * 15.0f + 0.5f);
			const uint8_t ig = static_cast<uint8_t>(Math::saturate(rgbaPtr[1]) * 15.0f + 0.5f);
			const uint8_t ib = static_cast<uint8_t>(Math::saturate(rgbaPtr[2]) * 15.0f + 0.5f);
			const uint8_t ia = static_cast<uint8_t>(Math::saturate(rgbaPtr[2]) * 15.0f + 0.5f);

			((uint16_t *)dstPtr)[0] = (ia << 12u) | (ir << 8u) | (ig << 4u) | (ib);
			break;
		}

		case PFG_RGB8_UNORM:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 3u, flags);
			break;
		case PFG_RGB8_UNORM_SRGB:
			convertFromFloat<uint8_t>(rgbaPtr, dstPtr, 3u, flags);
			break;
		case PFG_BGR8_UNORM:
			((uint8_t *)dstPtr)[0] = static_cast<uint8_t>(Math::saturate(rgbaPtr[2]) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[1] = static_cast<uint8_t>(Math::saturate(rgbaPtr[1]) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[2] = static_cast<uint8_t>(Math::saturate(rgbaPtr[0]) * 255.0f + 0.5f);
			break;
		case PFG_BGR8_UNORM_SRGB:
			((uint8_t *)dstPtr)[0] =
					static_cast<uint8_t>(Math::saturate(toSRGB(rgbaPtr[2])) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[1] =
					static_cast<uint8_t>(Math::saturate(toSRGB(rgbaPtr[1])) * 255.0f + 0.5f);
			((uint8_t *)dstPtr)[2] =
					static_cast<uint8_t>(Math::saturate(toSRGB(rgbaPtr[0])) * 255.0f + 0.5f);
			break;

		case PFG_RGB16_UNORM:
			convertFromFloat<uint16_t>(rgbaPtr, dstPtr, 3u, flags);
			break;

		case PFG_AYUV:
		case PFG_Y410:
		case PFG_Y416:
		case PFG_NV12:
		case PFG_P010:
		case PFG_P016:
		case PFG_420_OPAQUE:
		case PFG_YUY2:
		case PFG_Y210:
		case PFG_Y216:
		case PFG_NV11:
		case PFG_AI44:
		case PFG_IA44:
		case PFG_P8:
		case PFG_A8P8:
		case PFG_P208:
		case PFG_V208:
		case PFG_V408:
		case PFG_UNKNOWN:
		case PFG_NULL:
		case PFG_COUNT:
			return false;
			break;

		case PFG_BC1_UNORM:
		case PFG_BC1_UNORM_SRGB:
		case PFG_BC2_UNORM:
		case PFG_BC2_UNORM_SRGB:
		case PFG_BC3_UNORM:
		case PFG_BC3_UNORM_SRGB:
		case PFG_BC4_UNORM:
		case PFG_BC4_SNORM:
		case PFG_BC5_UNORM:
		case PFG_BC5_SNORM:
		case PFG_BC6H_UF16:
		case PFG_BC6H_SF16:
		case PFG_BC7_UNORM:
		case PFG_BC7_UNORM_SRGB:
		case PFG_PVRTC_RGB2:
		case PFG_PVRTC_RGB2_SRGB:
		case PFG_PVRTC_RGBA2:
		case PFG_PVRTC_RGBA2_SRGB:
		case PFG_PVRTC_RGB4:
		case PFG_PVRTC_RGB4_SRGB:
		case PFG_PVRTC_RGBA4:
		case PFG_PVRTC_RGBA4_SRGB:
		case PFG_PVRTC2_2BPP:
		case PFG_PVRTC2_2BPP_SRGB:
		case PFG_PVRTC2_4BPP:
		case PFG_PVRTC2_4BPP_SRGB:
		case PFG_ETC1_RGB8_UNORM:
		case PFG_ETC2_RGB8_UNORM:
		case PFG_ETC2_RGB8_UNORM_SRGB:
		case PFG_ETC2_RGBA8_UNORM:
		case PFG_ETC2_RGBA8_UNORM_SRGB:
		case PFG_ETC2_RGB8A1_UNORM:
		case PFG_ETC2_RGB8A1_UNORM_SRGB:
		case PFG_EAC_R11_UNORM:
		case PFG_EAC_R11_SNORM:
		case PFG_EAC_R11G11_UNORM:
		case PFG_EAC_R11G11_SNORM:
		case PFG_ATC_RGB:
		case PFG_ATC_RGBA_EXPLICIT_ALPHA:
		case PFG_ATC_RGBA_INTERPOLATED_ALPHA:
		case PFG_ASTC_RGBA_UNORM_4X4_LDR:
		case PFG_ASTC_RGBA_UNORM_4X4_sRGB:
		case PFG_ASTC_RGBA_UNORM_5X4_LDR:
		case PFG_ASTC_RGBA_UNORM_5X4_sRGB:
		case PFG_ASTC_RGBA_UNORM_5X5_LDR:
		case PFG_ASTC_RGBA_UNORM_5X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_6X5_LDR:
		case PFG_ASTC_RGBA_UNORM_6X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_6X6_LDR:
		case PFG_ASTC_RGBA_UNORM_6X6_sRGB:
		case PFG_ASTC_RGBA_UNORM_8X5_LDR:
		case PFG_ASTC_RGBA_UNORM_8X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_8X6_LDR:
		case PFG_ASTC_RGBA_UNORM_8X6_sRGB:
		case PFG_ASTC_RGBA_UNORM_8X8_LDR:
		case PFG_ASTC_RGBA_UNORM_8X8_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X5_LDR:
		case PFG_ASTC_RGBA_UNORM_10X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X6_LDR:
		case PFG_ASTC_RGBA_UNORM_10X6_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X8_LDR:
		case PFG_ASTC_RGBA_UNORM_10X8_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X10_LDR:
		case PFG_ASTC_RGBA_UNORM_10X10_sRGB:
		case PFG_ASTC_RGBA_UNORM_12X10_LDR:
		case PFG_ASTC_RGBA_UNORM_12X10_sRGB:
		case PFG_ASTC_RGBA_UNORM_12X12_LDR:
		case PFG_ASTC_RGBA_UNORM_12X12_sRGB:
			return false;
			break;
	}
	return true;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::unpackColour(float *rgbaPtr, PixelFormatGpu pf, const void *srcPtr) {
	const uint32_t flags = getFlags(pf);
	switch (pf) {
		case PFG_RGBA32_FLOAT:
			convertToFloat<float>(rgbaPtr, srcPtr, 4u, flags);
			break;
		case PFG_RGBA32_UINT:
			convertToFloat<uint32_t>(rgbaPtr, srcPtr, 4u, flags);
			break;
		case PFG_RGBA32_SINT:
			convertToFloat<int32_t>(rgbaPtr, srcPtr, 4u, flags);
			break;
		case PFG_RGB32_FLOAT:
			convertToFloat<float>(rgbaPtr, srcPtr, 3u, flags);
			break;
		case PFG_RGB32_UINT:
			convertToFloat<uint32_t>(rgbaPtr, srcPtr, 3u, flags);
			break;
		case PFG_RGB32_SINT:
			convertToFloat<int32_t>(rgbaPtr, srcPtr, 3u, flags);
			break;
		case PFG_RGBA16_FLOAT:
			convertToFloat<uint16_t>(rgbaPtr, srcPtr, 4u, flags);
			break;
		case PFG_RGBA16_UNORM:
			convertToFloat<uint16_t>(rgbaPtr, srcPtr, 4u, flags);
			break;
		case PFG_RGBA16_UINT:
			convertToFloat<uint16_t>(rgbaPtr, srcPtr, 4u, flags);
			break;
		case PFG_RGBA16_SNORM:
			convertToFloat<int16_t>(rgbaPtr, srcPtr, 4u, flags);
			break;
		case PFG_RGBA16_SINT:
			convertToFloat<int16_t>(rgbaPtr, srcPtr, 4u, flags);
			break;
		case PFG_RG32_FLOAT:
			convertToFloat<float>(rgbaPtr, srcPtr, 2u, flags);
			break;
		case PFG_RG32_UINT:
			convertToFloat<uint32_t>(rgbaPtr, srcPtr, 2u, flags);
			break;
		case PFG_RG32_SINT:
			convertToFloat<int32_t>(rgbaPtr, srcPtr, 2u, flags);
			break;
		case PFG_D32_FLOAT_S8X24_UINT:
			rgbaPtr[0] = ((const float *)srcPtr)[0];
			rgbaPtr[1] = static_cast<float>(((const uint32_t *)srcPtr)[1] >> 24u);
			rgbaPtr[2] = 0.0f;
			rgbaPtr[3] = 1.0f;
			break;
		case PFG_R10G10B10A2_UNORM: {
			const uint32_t val = ((const uint32_t *)srcPtr)[0];
			rgbaPtr[0] = static_cast<float>(val & 0x3FF) / 1023.0f;
			rgbaPtr[1] = static_cast<float>((val >> 10u) & 0x3FF) / 1023.0f;
			rgbaPtr[2] = static_cast<float>((val >> 20u) & 0x3FF) / 1023.0f;
			rgbaPtr[3] = static_cast<float>(val >> 30u) / 3.0f;
			break;
		}
		case PFG_R10G10B10A2_UINT: {
			const uint32_t val = ((const uint32_t *)srcPtr)[0];
			rgbaPtr[0] = static_cast<float>(val & 0x3FF);
			rgbaPtr[1] = static_cast<float>((val >> 10u) & 0x3FF);
			rgbaPtr[2] = static_cast<float>((val >> 20u) & 0x3FF);
			rgbaPtr[3] = static_cast<float>(val >> 30u);
			break;
		}
		case PFG_R11G11B10_FLOAT:
			return false;
			break;
		case PFG_RGBA8_UNORM:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 4u, flags);
			break;
		case PFG_RGBA8_UNORM_SRGB:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 4u, flags);
			break;
		case PFG_RGBA8_UINT:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 4u, flags);
			break;
		case PFG_RGBA8_SNORM:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 4u, flags);
			break;
		case PFG_RGBA8_SINT:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 4u, flags);
			break;
		case PFG_RG16_FLOAT:
			convertToFloat<uint16_t>(rgbaPtr, srcPtr, 2u, flags);
			break;
		case PFG_RG16_UNORM:
			convertToFloat<uint16_t>(rgbaPtr, srcPtr, 2u, flags);
			break;
		case PFG_RG16_UINT:
			convertToFloat<uint16_t>(rgbaPtr, srcPtr, 2u, flags);
			break;
		case PFG_RG16_SNORM:
			convertToFloat<int16_t>(rgbaPtr, srcPtr, 2u, flags);
			break;
		case PFG_RG16_SINT:
			convertToFloat<int16_t>(rgbaPtr, srcPtr, 2u, flags);
			break;
		case PFG_D32_FLOAT:
			convertToFloat<float>(rgbaPtr, srcPtr, 1u, flags);
			break;
		case PFG_R32_FLOAT:
			convertToFloat<float>(rgbaPtr, srcPtr, 1u, flags);
			break;
		case PFG_R32_UINT:
			convertToFloat<uint32_t>(rgbaPtr, srcPtr, 1u, flags);
			break;
		case PFG_R32_SINT:
			convertToFloat<int32_t>(rgbaPtr, srcPtr, 1u, flags);
			break;
		case PFG_D24_UNORM:
			rgbaPtr[0] = static_cast<float>(((const uint32_t *)srcPtr)[0]) / 16777215.0f;
			rgbaPtr[1] = 0.0f;
			rgbaPtr[2] = 0.0f;
			rgbaPtr[3] = 1.0f;
			break;
		case PFG_D24_UNORM_S8_UINT:
			rgbaPtr[0] = static_cast<float>(((const uint32_t *)srcPtr)[0] & 0x00FFFFFF) / 16777215.0f;
			rgbaPtr[1] = static_cast<float>(((const uint32_t *)srcPtr)[0] >> 24u);
			rgbaPtr[2] = 0.0f;
			rgbaPtr[3] = 1.0f;
			break;
		case PFG_RG8_UNORM:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 2u, flags);
			break;
		case PFG_RG8_UINT:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 2u, flags);
			break;
		case PFG_RG8_SNORM:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 2u, flags);
			break;
		case PFG_RG8_SINT:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 2u, flags);
			break;
		case PFG_R16_FLOAT:
			convertToFloat<uint16_t>(rgbaPtr, srcPtr, 1u, flags);
			break;
		case PFG_D16_UNORM:
			convertToFloat<uint16_t>(rgbaPtr, srcPtr, 1u, flags);
			break;
		case PFG_R16_UNORM:
			convertToFloat<uint16_t>(rgbaPtr, srcPtr, 1u, flags);
			break;
		case PFG_R16_UINT:
			convertToFloat<uint16_t>(rgbaPtr, srcPtr, 1u, flags);
			break;
		case PFG_R16_SNORM:
			convertToFloat<int16_t>(rgbaPtr, srcPtr, 1u, flags);
			break;
		case PFG_R16_SINT:
			convertToFloat<int16_t>(rgbaPtr, srcPtr, 1u, flags);
			break;
		case PFG_R8_UNORM:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 1u, flags);
			break;
		case PFG_R8_UINT:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 1u, flags);
			break;
		case PFG_R8_SNORM:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 1u, flags);
			break;
		case PFG_R8_SINT:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 1u, flags);
			break;
		case PFG_A8_UNORM:
			rgbaPtr[0] = 0;
			rgbaPtr[1] = 0;
			rgbaPtr[2] = 0;
			rgbaPtr[3] = static_cast<float>(((const uint32_t *)srcPtr)[0]);
			break;
		case PFG_R1_UNORM:
		case PFG_R9G9B9E5_SHAREDEXP:
		case PFG_R8G8_B8G8_UNORM:
		case PFG_G8R8_G8B8_UNORM:
			return false;
			break;
		case PFG_B5G6R5_UNORM: {
			const uint16_t val = ((const uint16_t *)srcPtr)[0];
			rgbaPtr[0] = static_cast<float>((val >> 11u) & 0x1F) / 31.0f;
			rgbaPtr[1] = static_cast<float>((val >> 5u) & 0x3F) / 63.0f;
			rgbaPtr[2] = static_cast<float>(val & 0x1F) / 31.0f;
			rgbaPtr[3] = 1.0f;
			break;
		}
		case PFG_B5G5R5A1_UNORM: {
			const uint16_t val = ((const uint16_t *)srcPtr)[0];
			rgbaPtr[0] = static_cast<float>((val >> 10u) & 0x1F) / 31.0f;
			rgbaPtr[1] = static_cast<float>((val >> 5u) & 0x1F) / 31.0f;
			rgbaPtr[2] = static_cast<float>(val & 0x1F) / 31.0f;
			rgbaPtr[3] = (val >> 15u) == 0 ? 0.0f : 1.0f;
			break;
		}
		case PFG_BGRA8_UNORM:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 4u, flags);
			std::swap(rgbaPtr[0], rgbaPtr[2]);
			break;
		case PFG_BGRX8_UNORM:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 4u, flags);
			std::swap(rgbaPtr[0], rgbaPtr[2]);
			break;
		case PFG_R10G10B10_XR_BIAS_A2_UNORM:
			return false;
			break;
		case PFG_BGRA8_UNORM_SRGB:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 4u, flags);
			std::swap(rgbaPtr[0], rgbaPtr[2]);
			break;
		case PFG_BGRX8_UNORM_SRGB:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 3u, flags);
			std::swap(rgbaPtr[0], rgbaPtr[2]);
			break;
		case PFG_B4G4R4A4_UNORM: {
			const uint16_t val = ((const uint16_t *)srcPtr)[0];
			rgbaPtr[0] = static_cast<float>((val >> 8u) & 0xF) / 15.0f;
			rgbaPtr[1] = static_cast<float>((val >> 4u) & 0xF) / 15.0f;
			rgbaPtr[2] = static_cast<float>(val & 0xF) / 15.0f;
			rgbaPtr[3] = static_cast<float>((val >> 12u) & 0xF) / 15.0f;
			break;
		}

		case PFG_RGB8_UNORM:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 3u, flags);
			break;
		case PFG_RGB8_UNORM_SRGB:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 3u, flags);
			break;
		case PFG_BGR8_UNORM:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 3u, flags);
			std::swap(rgbaPtr[0], rgbaPtr[2]);
			break;
		case PFG_BGR8_UNORM_SRGB:
			convertToFloat<uint8_t>(rgbaPtr, srcPtr, 3u, flags);
			std::swap(rgbaPtr[0], rgbaPtr[2]);
			break;

		case PFG_RGB16_UNORM:
			convertToFloat<uint16_t>(rgbaPtr, srcPtr, 3u, flags);
			break;

		case PFG_AYUV:
		case PFG_Y410:
		case PFG_Y416:
		case PFG_NV12:
		case PFG_P010:
		case PFG_P016:
		case PFG_420_OPAQUE:
		case PFG_YUY2:
		case PFG_Y210:
		case PFG_Y216:
		case PFG_NV11:
		case PFG_AI44:
		case PFG_IA44:
		case PFG_P8:
		case PFG_A8P8:
		case PFG_P208:
		case PFG_V208:
		case PFG_V408:
		case PFG_UNKNOWN:
		case PFG_NULL:
		case PFG_COUNT:
			return false;
			break;

		case PFG_BC1_UNORM:
		case PFG_BC1_UNORM_SRGB:
		case PFG_BC2_UNORM:
		case PFG_BC2_UNORM_SRGB:
		case PFG_BC3_UNORM:
		case PFG_BC3_UNORM_SRGB:
		case PFG_BC4_UNORM:
		case PFG_BC4_SNORM:
		case PFG_BC5_UNORM:
		case PFG_BC5_SNORM:
		case PFG_BC6H_UF16:
		case PFG_BC6H_SF16:
		case PFG_BC7_UNORM:
		case PFG_BC7_UNORM_SRGB:
		case PFG_PVRTC_RGB2:
		case PFG_PVRTC_RGB2_SRGB:
		case PFG_PVRTC_RGBA2:
		case PFG_PVRTC_RGBA2_SRGB:
		case PFG_PVRTC_RGB4:
		case PFG_PVRTC_RGB4_SRGB:
		case PFG_PVRTC_RGBA4:
		case PFG_PVRTC_RGBA4_SRGB:
		case PFG_PVRTC2_2BPP:
		case PFG_PVRTC2_2BPP_SRGB:
		case PFG_PVRTC2_4BPP:
		case PFG_PVRTC2_4BPP_SRGB:
		case PFG_ETC1_RGB8_UNORM:
		case PFG_ETC2_RGB8_UNORM:
		case PFG_ETC2_RGB8_UNORM_SRGB:
		case PFG_ETC2_RGBA8_UNORM:
		case PFG_ETC2_RGBA8_UNORM_SRGB:
		case PFG_ETC2_RGB8A1_UNORM:
		case PFG_ETC2_RGB8A1_UNORM_SRGB:
		case PFG_EAC_R11_UNORM:
		case PFG_EAC_R11_SNORM:
		case PFG_EAC_R11G11_UNORM:
		case PFG_EAC_R11G11_SNORM:
		case PFG_ATC_RGB:
		case PFG_ATC_RGBA_EXPLICIT_ALPHA:
		case PFG_ATC_RGBA_INTERPOLATED_ALPHA:
		case PFG_ASTC_RGBA_UNORM_4X4_LDR:
		case PFG_ASTC_RGBA_UNORM_4X4_sRGB:
		case PFG_ASTC_RGBA_UNORM_5X4_LDR:
		case PFG_ASTC_RGBA_UNORM_5X4_sRGB:
		case PFG_ASTC_RGBA_UNORM_5X5_LDR:
		case PFG_ASTC_RGBA_UNORM_5X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_6X5_LDR:
		case PFG_ASTC_RGBA_UNORM_6X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_6X6_LDR:
		case PFG_ASTC_RGBA_UNORM_6X6_sRGB:
		case PFG_ASTC_RGBA_UNORM_8X5_LDR:
		case PFG_ASTC_RGBA_UNORM_8X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_8X6_LDR:
		case PFG_ASTC_RGBA_UNORM_8X6_sRGB:
		case PFG_ASTC_RGBA_UNORM_8X8_LDR:
		case PFG_ASTC_RGBA_UNORM_8X8_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X5_LDR:
		case PFG_ASTC_RGBA_UNORM_10X5_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X6_LDR:
		case PFG_ASTC_RGBA_UNORM_10X6_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X8_LDR:
		case PFG_ASTC_RGBA_UNORM_10X8_sRGB:
		case PFG_ASTC_RGBA_UNORM_10X10_LDR:
		case PFG_ASTC_RGBA_UNORM_10X10_sRGB:
		case PFG_ASTC_RGBA_UNORM_12X10_LDR:
		case PFG_ASTC_RGBA_UNORM_12X10_sRGB:
		case PFG_ASTC_RGBA_UNORM_12X12_LDR:
		case PFG_ASTC_RGBA_UNORM_12X12_sRGB:
			return false;
			break;
	}
	return true;
}
//-----------------------------------------------------------------------------------
void PixelFormatGpuUtils::packColour(const Color &rgbaPtr, PixelFormatGpu pf, void *dstPtr) {
	float tmpVal[4];
	tmpVal[0] = rgbaPtr.r;
	tmpVal[1] = rgbaPtr.g;
	tmpVal[2] = rgbaPtr.b;
	tmpVal[3] = rgbaPtr.a;
	packColour(tmpVal, pf, dstPtr);
}
//-----------------------------------------------------------------------------------
void PixelFormatGpuUtils::unpackColour(Color *rgbaPtr, PixelFormatGpu pf, const void *srcPtr) {
	float tmpVal[4];
	unpackColour(tmpVal, pf, srcPtr);
	rgbaPtr->r = tmpVal[0];
	rgbaPtr->g = tmpVal[1];
	rgbaPtr->b = tmpVal[2];
	rgbaPtr->a = tmpVal[3];
}
//-----------------------------------------------------------------------------------
void PixelFormatGpuUtils::convertForNormalMapping(TextureBox src, PixelFormatGpu srcFormat,
		TextureBox dst, PixelFormatGpu dstFormat) {
	assert(src.equalSize(dst));
	assert(dstFormat == PFG_RG8_SNORM || dstFormat == PFG_RG8_UNORM);

	if (srcFormat == PFG_RGBA8_UNORM || srcFormat == PFG_RGBA8_SNORM || srcFormat == PFG_BGRA8_UNORM || srcFormat == PFG_BGRX8_UNORM || srcFormat == PFG_RGB8_UNORM || srcFormat == PFG_BGR8_UNORM || srcFormat == PFG_RG8_UNORM || srcFormat == PFG_RG8_SNORM) {
		bulkPixelConversion(src, srcFormat, dst, dstFormat);
		return;
	}

	float multPart = 2.0f;
	float addPart = 1.0f;

	if (dstFormat == PFG_RG8_UNORM) {
		multPart = 1.0f;
		addPart = 0.0f;
	}

	float rgba[4];
	for (size_t z = 0; z < src.getDepthOrSlices(); ++z) {
		for (size_t y = 0; y < src.height; ++y) {
			uint8_t const *srcPtr = reinterpret_cast<const uint8_t *>(
					src.atFromOffsettedOrigin(0, y, z));
			uint8_t *dstPtr = reinterpret_cast<uint8_t *>(
					dst.atFromOffsettedOrigin(0, y, z));

			for (size_t x = 0; x < src.width; ++x) {
				unpackColour(rgba, srcFormat, srcPtr);

				// rgba * 2.0 - 1.0
				rgba[0] = rgba[0] * multPart - addPart;
				rgba[1] = rgba[1] * multPart - addPart;

				*dstPtr++ = Math::floatToSnorm8(rgba[0]);
				*dstPtr++ = Math::floatToSnorm8(rgba[1]);

				srcPtr += src.bytesPerPixel;
			}
		}
	}
}
//-----------------------------------------------------------------------------------
namespace {
typedef void (*row_conversion_func_t)(uint8_t *src, uint8_t *dst, uint32_t width);

void convCopy16Bpx(uint8_t *src, uint8_t *dst, uint32_t width) {
	memcpy(dst, src, 16 * width);
}
void convCopy12Bpx(uint8_t *src, uint8_t *dst, uint32_t width) {
	memcpy(dst, src, 12 * width);
}
void convCopy8Bpx(uint8_t *src, uint8_t *dst, uint32_t width) {
	memcpy(dst, src, 8 * width);
}
void convCopy6Bpx(uint8_t *src, uint8_t *dst, uint32_t width) {
	memcpy(dst, src, 6 * width);
}
void convCopy4Bpx(uint8_t *src, uint8_t *dst, uint32_t width) {
	memcpy(dst, src, 4 * width);
}
void convCopy3Bpx(uint8_t *src, uint8_t *dst, uint32_t width) {
	memcpy(dst, src, 3 * width);
}
void convCopy2Bpx(uint8_t *src, uint8_t *dst, uint32_t width) {
	memcpy(dst, src, 2 * width);
}
void convCopy1Bpx(uint8_t *src, uint8_t *dst, uint32_t width) {
	memcpy(dst, src, 1 * width);
}

// clang-format off
        void convRGBA32toRGB32(uint8_t* _src, uint8_t* _dst, uint32_t width) {
            uint32_t* src = (uint32_t*)_src; uint32_t* dst = (uint32_t*)_dst;
            while (width--) { dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; src += 4; dst += 3; }
        }
        void convRGB32toRG32(uint8_t* _src, uint8_t* _dst, uint32_t width) {
            uint32_t* src = (uint32_t*)_src; uint32_t* dst = (uint32_t*)_dst;
            while (width--) { dst[0] = src[0]; dst[1] = src[1]; src += 3; dst += 2; }
        }
        void convRG32toRGB32(uint8_t* _src, uint8_t* _dst, uint32_t width) {
            uint32_t* src = (uint32_t*)_src; uint32_t* dst = (uint32_t*)_dst;
            while (width--) { dst[0] = src[0]; dst[1] = src[1]; dst[2] = 0u; src += 2; dst += 3; }
        }
        void convRG32toR32(uint8_t* _src, uint8_t* _dst, uint32_t width) {
            uint32_t* src = (uint32_t*)_src; uint32_t* dst = (uint32_t*)_dst;
            while (width--) { dst[0] = src[0]; src += 2; dst += 1; }
        }

        void convRGBA16toRGB16(uint8_t* _src, uint8_t* _dst, uint32_t width) {
            uint16_t* src = (uint16_t*)_src; uint16_t* dst = (uint16_t*)_dst;
            while (width--) { dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; src += 4; dst += 3; }
        }
        void convRGB16toRGBA16(uint8_t* _src, uint8_t* _dst, uint32_t width) {
            uint16_t* src = (uint16_t*)_src; uint16_t* dst = (uint16_t*)_dst;
            while (width--)
            { dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = 0xFFFF; src += 3; dst += 4; }
        }
        void convRGB16toRG16(uint8_t* _src, uint8_t* _dst, uint32_t width) {
            uint16_t* src = (uint16_t*)_src; uint16_t* dst = (uint16_t*)_dst;
            while (width--) { dst[0] = src[0]; dst[1] = src[1]; src += 3; dst += 2; }
        }
        void convRG16toRGB16(uint8_t* _src, uint8_t* _dst, uint32_t width) {
            uint16_t* src = (uint16_t*)_src; uint16_t* dst = (uint16_t*)_dst;
            while (width--) { dst[0] = src[0]; dst[1] = src[1]; dst[0] = 0u; src += 2; dst += 3; }
        }
        void convRG16toR16(uint8_t* _src, uint8_t* _dst, uint32_t width) {
            uint16_t* src = (uint16_t*)_src; uint16_t* dst = (uint16_t*)_dst;
            while (width--) { dst[0] = src[0]; src += 2; dst += 1; }
        }

        void convRGBAtoBGRA(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--)
            { dst[0] = src[2]; dst[1] = src[1]; dst[2] = src[0]; dst[3] = src[3]; src += 4; dst += 4; }
        }
        void convRGBAtoRGB(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; src += 4; dst += 3; }
        }
        void convRGBAtoBGR(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[2]; dst[1] = src[1]; dst[2] = src[0]; src += 4; dst += 3; }
        }
        void convRGBAtoRG(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[0]; dst[1] = src[1]; src += 4; dst += 2; }
        }
        void convRGBAtoRG_u2s(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[0] - 128; dst[1] = src[1] - 128; src += 4; dst += 2; }
        }
        void convRGBAtoRG_s2u(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[0] + 128; dst[1] = src[1] + 128; src += 4; dst += 2; }
        }
        void convRGBAtoR(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[0]; src += 4; dst += 1; }
        }

        void convBGRAtoRG(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[2]; dst[1] = src[1]; src += 4; dst += 2; }
        }
        void convBGRAtoRG_u2s(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[2] - 128; dst[1] = src[1] - 128; src += 4; dst += 2; }
        }
        void convBGRAtoRG_s2u(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[2] + 128; dst[1] = src[1] + 128; src += 4; dst += 2; }
        }
        void convBGRAtoR(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[2]; src += 4; dst += 1; }
        }

        void convBGRXtoRGBA(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--)
            { dst[0] = src[2]; dst[1] = src[1]; dst[2] = src[0]; dst[3] = 0xFF; src += 4; dst += 4; }
        }
        void convBGRXtoBGRA(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--)
            { dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = 0xFF; src += 4; dst += 4; }
        }

        void convRGBtoRGBA(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--)
            { dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = 0xFF; src += 3; dst += 4; }
        }
        void convRGBtoBGRA(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--)
            { dst[0] = src[2]; dst[1] = src[1]; dst[2] = src[0]; dst[3] = 0xFF; src += 3; dst += 4; }
        }
        void convRGBtoBGR(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[2]; dst[1] = src[1]; dst[2] = src[0]; src += 3; dst += 3; }
        }
        void convRGBtoRG(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[0]; dst[1] = src[1]; src += 3; dst += 2; }
        }
        void convRGBtoRG_u2s(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[0] - 128; dst[1] = src[1] - 128; src += 3; dst += 2; }
        }
        void convRGBtoRG_s2u(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[0] + 128; dst[1] = src[1] + 128; src += 3; dst += 2; }
        }
        void convRGBtoR(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[0]; src += 3; dst += 1; }
        }

        void convBGRtoRG(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[2]; dst[1] = src[1]; src += 3; dst += 2; }
        }
        void convBGRtoRG_u2s(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[2] - 128; dst[1] = src[1] - 128; src += 3; dst += 2; }
        }
        void convBGRtoRG_s2u(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[2] + 128; dst[1] = src[1] + 128; src += 3; dst += 2; }
        }
        void convBGRtoR(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[2]; src += 3; dst += 1; }
        }

        void convRGtoRGB(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[0]; dst[1] = src[1]; dst[2] = 0u; src += 2; dst += 3; }
        }
        void convRGtoBGR(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = 0u; dst[1] = src[1]; dst[2] = src[0]; src += 2; dst += 3; }
        }
        void convRGtoRG_u2s(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[0] - 128; dst[1] = src[1] - 128; src += 2; dst += 2; }
        }
        void convRGtoRG_s2u(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[0] + 128; dst[1] = src[1] + 128; src += 2; dst += 2; }
        }
        void convRGtoR(uint8_t* src, uint8_t* dst, uint32_t width) {
            while (width--) { dst[0] = src[0]; src += 2; dst += 1; }
        }
// clang-format on
} // namespace
//-----------------------------------------------------------------------------------
void PixelFormatGpuUtils::bulkPixelConversion(const TextureBox &src, PixelFormatGpu srcFormat,
		TextureBox &dst, PixelFormatGpu dstFormat,
		bool verticalFlip) {
	if (srcFormat == dstFormat && !verticalFlip) {
		dst.copyFrom(src);
		return;
	}

	if (isCompressed(srcFormat) || isCompressed(dstFormat)) {
		return;
	}

	assert(src.equalSize(dst));
	assert(getBytesPerPixel(srcFormat) == src.bytesPerPixel);
	assert(getBytesPerPixel(dstFormat) == dst.bytesPerPixel);

	const size_t srcBytesPerPixel = src.bytesPerPixel;
	const size_t dstBytesPerPixel = dst.bytesPerPixel;

	uint8_t *srcData = reinterpret_cast<uint8_t *>(src.at(src.x, src.y, src.getZOrSlice()));
	uint8_t *dstData = reinterpret_cast<uint8_t *>(dst.at(dst.x, dst.y, dst.getZOrSlice()));

	const size_t width = src.width;
	const size_t height = src.height;
	const size_t depthOrSlices = src.getDepthOrSlices();

	// Is there a optimized row conversion?
	row_conversion_func_t rowConversionFunc = 0;
	assert(PFL_COUNT <= 16); // adjust PFL_PAIR definition if assertion failed
#define PFL_PAIR(a, b) ((a << 4) | b)
	if (srcFormat == dstFormat) {
		switch (srcBytesPerPixel) {
				// clang-format off
            case 1: rowConversionFunc = convCopy1Bpx; break;
            case 2: rowConversionFunc = convCopy2Bpx; break;
            case 3: rowConversionFunc = convCopy3Bpx; break;
            case 4: rowConversionFunc = convCopy4Bpx; break;
            case 6: rowConversionFunc = convCopy6Bpx; break;
            case 8: rowConversionFunc = convCopy8Bpx; break;
            case 12: rowConversionFunc = convCopy12Bpx; break;
            case 16: rowConversionFunc = convCopy16Bpx; break;
				// clang-format on
		}
	} else if (getFlags(srcFormat) == getFlags(dstFormat)) // semantic match, copy as typeless
	{
		PixelFormatLayout srcLayout = getPixelLayout(srcFormat);
		PixelFormatLayout dstLayout = getPixelLayout(dstFormat);
		switch (PFL_PAIR(srcLayout, dstLayout)) {
				// clang-format off
            case PFL_PAIR( PFL_RGBA32, PFL_RGB32 ): rowConversionFunc = convRGBA32toRGB32; break;
            case PFL_PAIR( PFL_RGB32, PFL_RG32 ): rowConversionFunc = convRGB32toRG32; break;
            case PFL_PAIR( PFL_RG32, PFL_RGB32 ): rowConversionFunc = convRG32toRGB32; break;
            case PFL_PAIR( PFL_RG32, PFL_R32 ): rowConversionFunc = convRG32toR32; break;

            case PFL_PAIR( PFL_RGBA16, PFL_RGB16 ): rowConversionFunc = convRGBA16toRGB16; break;
            case PFL_PAIR( PFL_RGB16, PFL_RGBA16 ): rowConversionFunc = convRGB16toRGBA16; break;
            case PFL_PAIR( PFL_RGB16, PFL_RG16 ): rowConversionFunc = convRGB16toRG16; break;
            case PFL_PAIR( PFL_RG16, PFL_RGB16 ): rowConversionFunc = convRG16toRGB16; break;
            case PFL_PAIR( PFL_RG16, PFL_R16 ): rowConversionFunc = convRG16toR16; break;

            case PFL_PAIR( PFL_RGBA8, PFL_BGRA8 ): rowConversionFunc = convRGBAtoBGRA; break;
            case PFL_PAIR( PFL_RGBA8, PFL_BGRX8 ): rowConversionFunc = convRGBAtoBGRA; break;
            case PFL_PAIR( PFL_RGBA8, PFL_RGB8 ): rowConversionFunc = convRGBAtoRGB; break;
            case PFL_PAIR( PFL_RGBA8, PFL_BGR8 ): rowConversionFunc = convRGBAtoBGR; break;
            case PFL_PAIR( PFL_RGBA8, PFL_RG8 ): rowConversionFunc = convRGBAtoRG; break;
            case PFL_PAIR( PFL_RGBA8, PFL_R8 ): rowConversionFunc = convRGBAtoR; break;

            case PFL_PAIR( PFL_BGRA8, PFL_RGBA8 ): rowConversionFunc = convRGBAtoBGRA; break;
            case PFL_PAIR( PFL_BGRA8, PFL_BGRX8 ): rowConversionFunc = convCopy4Bpx; break;
            case PFL_PAIR( PFL_BGRA8, PFL_RGB8 ): rowConversionFunc = convRGBAtoBGR; break;
            case PFL_PAIR( PFL_BGRA8, PFL_BGR8 ): rowConversionFunc = convRGBAtoRGB; break;
            case PFL_PAIR( PFL_BGRA8, PFL_RG8 ): rowConversionFunc = convBGRAtoRG; break;
            case PFL_PAIR( PFL_BGRA8, PFL_R8 ): rowConversionFunc = convBGRAtoR; break;

            case PFL_PAIR( PFL_BGRX8, PFL_RGBA8 ): rowConversionFunc = convBGRXtoRGBA; break;
            case PFL_PAIR( PFL_BGRX8, PFL_BGRA8 ): rowConversionFunc = convBGRXtoBGRA; break;
            case PFL_PAIR( PFL_BGRX8, PFL_RGB8 ): rowConversionFunc = convRGBAtoBGR; break;
            case PFL_PAIR( PFL_BGRX8, PFL_BGR8 ): rowConversionFunc = convRGBAtoRGB; break;
            case PFL_PAIR( PFL_BGRX8, PFL_RG8 ): rowConversionFunc = convBGRAtoRG; break;
            case PFL_PAIR( PFL_BGRX8, PFL_R8 ): rowConversionFunc = convBGRAtoR; break;

            case PFL_PAIR( PFL_RGB8, PFL_RGBA8 ): rowConversionFunc = convRGBtoRGBA; break;
            case PFL_PAIR( PFL_RGB8, PFL_BGRA8 ): rowConversionFunc = convRGBtoBGRA; break;
            case PFL_PAIR( PFL_RGB8, PFL_BGRX8 ): rowConversionFunc = convRGBtoBGRA; break;
            case PFL_PAIR( PFL_RGB8, PFL_BGR8 ): rowConversionFunc = convRGBtoBGR; break;
            case PFL_PAIR( PFL_RGB8, PFL_RG8 ): rowConversionFunc = convRGBtoRG; break;
            case PFL_PAIR( PFL_RGB8, PFL_R8 ): rowConversionFunc = convRGBtoR; break;

            case PFL_PAIR( PFL_BGR8, PFL_RGBA8 ): rowConversionFunc = convRGBtoBGRA; break;
            case PFL_PAIR( PFL_BGR8, PFL_BGRA8 ): rowConversionFunc = convRGBtoRGBA; break;
            case PFL_PAIR( PFL_BGR8, PFL_BGRX8 ): rowConversionFunc = convRGBtoRGBA; break;
            case PFL_PAIR( PFL_BGR8, PFL_RGB8 ): rowConversionFunc = convRGBAtoBGR; break;
            case PFL_PAIR( PFL_BGR8, PFL_RG8 ): rowConversionFunc = convBGRtoRG; break;
            case PFL_PAIR( PFL_BGR8, PFL_R8 ): rowConversionFunc = convBGRtoR; break;

            case PFL_PAIR( PFL_RG8, PFL_RGB8 ): rowConversionFunc = convRGtoRGB; break;
            case PFL_PAIR( PFL_RG8, PFL_BGR8 ): rowConversionFunc = convRGtoBGR; break;
            case PFL_PAIR( PFL_RG8, PFL_R8 ): rowConversionFunc = convRGtoR; break;
				// clang-format on
		}
	} else if (getFlags(srcFormat) == PFF_NORMALIZED && getFlags(dstFormat) == (PFF_NORMALIZED | PFF_SIGNED)) {
		PixelFormatLayout srcLayout = getPixelLayout(srcFormat);
		PixelFormatLayout dstLayout = getPixelLayout(dstFormat);
		switch (PFL_PAIR(srcLayout, dstLayout)) {
				// clang-format off
            case PFL_PAIR( PFL_RGBA8, PFL_RG8 ): rowConversionFunc = convRGBAtoRG_u2s; break;
            case PFL_PAIR( PFL_BGRA8, PFL_RG8 ): rowConversionFunc = convBGRAtoRG_u2s; break;
            case PFL_PAIR( PFL_BGRX8, PFL_RG8 ): rowConversionFunc = convBGRAtoRG_u2s; break;
            case PFL_PAIR( PFL_RGB8, PFL_RG8 ): rowConversionFunc = convRGBtoRG_u2s; break;
            case PFL_PAIR( PFL_BGR8, PFL_RG8 ): rowConversionFunc = convBGRtoRG_u2s; break;
            case PFL_PAIR( PFL_RG8, PFL_RG8 ): rowConversionFunc = convRGtoRG_u2s; break;
				// clang-format on
		}
	} else if (getFlags(srcFormat) == (PFF_NORMALIZED | PFF_SIGNED) && getFlags(dstFormat) == PFF_NORMALIZED) {
		PixelFormatLayout srcLayout = getPixelLayout(srcFormat);
		PixelFormatLayout dstLayout = getPixelLayout(dstFormat);
		switch (PFL_PAIR(srcLayout, dstLayout)) {
				// clang-format off
            case PFL_PAIR( PFL_RGBA8, PFL_RG8 ): rowConversionFunc = convRGBAtoRG_s2u; break;
            case PFL_PAIR( PFL_BGRA8, PFL_RG8 ): rowConversionFunc = convBGRAtoRG_s2u; break;
            case PFL_PAIR( PFL_BGRX8, PFL_RG8 ): rowConversionFunc = convBGRAtoRG_s2u; break;
            case PFL_PAIR( PFL_RGB8, PFL_RG8 ): rowConversionFunc = convRGBtoRG_s2u; break;
            case PFL_PAIR( PFL_BGR8, PFL_RG8 ): rowConversionFunc = convBGRtoRG_s2u; break;
            case PFL_PAIR( PFL_RG8, PFL_RG8 ): rowConversionFunc = convRGtoRG_s2u; break;
				// clang-format on
		}
	}
#undef PFL_PAIR

	if (rowConversionFunc) {
		for (size_t z = 0; z < depthOrSlices; ++z) {
			for (size_t y = 0; y < height; ++y) {
				size_t dest_y = verticalFlip ? height - 1 - y : y;
				uint8_t *srcPtr = srcData + src.bytesPerImage * z + src.bytesPerRow * y;
				uint8_t *dstPtr = dstData + dst.bytesPerImage * z + dst.bytesPerRow * dest_y;
				rowConversionFunc(srcPtr, dstPtr, width);
			}
		}
		return;
	}

	// The brute force fallback
	float rangeM = 1.0f;
	float rangeA = 0.0f;

	const bool bSrcSigned = isSigned(srcFormat);
	if (bSrcSigned != isSigned(dstFormat) && isNormalized(srcFormat)) {
		if (!bSrcSigned) {
			// unormToSnorm
			rangeM = 2.0f;
			rangeA = -1.0f;
		} else {
			// snormToUnorm
			rangeM = 0.5f;
			rangeA = 0.5f;
		}
	}

	float rgba[4];
	for (size_t z = 0; z < depthOrSlices; ++z) {
		for (size_t y = 0; y < height; ++y) {
			size_t dest_y = verticalFlip ? height - 1 - y : y;
			uint8_t *srcPtr = srcData + src.bytesPerImage * z + src.bytesPerRow * y;
			uint8_t *dstPtr = dstData + dst.bytesPerImage * z + dst.bytesPerRow * dest_y;

			for (size_t x = 0; x < width; ++x) {
				unpackColour(rgba, srcFormat, srcPtr);
				for (int i = 0; i < 4; ++i)
					rgba[i] = rgba[i] * rangeM + rangeA;
				packColour(rgba, dstFormat, dstPtr);
				srcPtr += srcBytesPerPixel;
				dstPtr += dstBytesPerPixel;
			}
		}
	}
}
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
uint32_t PixelFormatGpuUtils::getFlags(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return desc.flags;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::isFloat(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return (desc.flags & PFF_FLOAT) != 0;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::isHalf(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return (desc.flags & PFF_HALF) != 0;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::isFloatRare(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return (desc.flags & PFF_FLOAT_RARE) != 0;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::isInteger(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return (desc.flags & PFF_INTEGER) != 0;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::isNormalized(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return (desc.flags & PFF_NORMALIZED) != 0;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::isSigned(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return (desc.flags & PFF_SIGNED) != 0;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::isDepth(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return (desc.flags & PFF_DEPTH) != 0;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::isStencil(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return (desc.flags & PFF_STENCIL) != 0;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::isSRgb(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return (desc.flags & PFF_SRGB) != 0;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::isCompressed(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return (desc.flags & PFF_COMPRESSED) != 0;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::isPallete(PixelFormatGpu format) {
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return (desc.flags & PFF_PALLETE) != 0;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::isAccessible(PixelFormatGpu format) {
	if (format == PFG_UNKNOWN)
		return false;
	const PixelFormatDesc &desc = getDescriptionFor(format);
	return (desc.flags & (PFF_COMPRESSED | PFF_DEPTH | PFF_STENCIL)) == 0;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::hasAlpha(PixelFormatGpu format) {
	return (getNumberOfComponents(format) == 4u && format != PFG_R8G8_B8G8_UNORM &&
				   format != PFG_G8R8_G8B8_UNORM) ||
		   format == PFG_A8_UNORM || format == PFG_A8P8 || format == PFG_IA44 || format == PFG_AI44;
}
//-----------------------------------------------------------------------------------
bool PixelFormatGpuUtils::hasSRGBEquivalent(PixelFormatGpu format) {
	return getEquivalentSRGB(format) != getEquivalentLinear(format);
}
//-----------------------------------------------------------------------------------
PixelFormatGpu PixelFormatGpuUtils::getEquivalentSRGB(PixelFormatGpu format) {
	switch (format) {
		case PFG_RGBA8_UNORM:
			return PFG_RGBA8_UNORM_SRGB;
		case PFG_BC1_UNORM:
			return PFG_BC1_UNORM_SRGB;
		case PFG_BC2_UNORM:
			return PFG_BC2_UNORM_SRGB;
		case PFG_BC3_UNORM:
			return PFG_BC3_UNORM_SRGB;
		case PFG_BGRA8_UNORM:
			return PFG_BGRA8_UNORM_SRGB;
		case PFG_BGRX8_UNORM:
			return PFG_BGRX8_UNORM_SRGB;
		case PFG_BC7_UNORM:
			return PFG_BC7_UNORM_SRGB;
		case PFG_RGB8_UNORM:
			return PFG_RGB8_UNORM_SRGB;
		case PFG_BGR8_UNORM:
			return PFG_BGR8_UNORM_SRGB;
		case PFG_ETC2_RGB8_UNORM:
			return PFG_ETC2_RGB8_UNORM_SRGB;
		case PFG_ETC2_RGBA8_UNORM:
			return PFG_ETC2_RGBA8_UNORM_SRGB;
		case PFG_ETC2_RGB8A1_UNORM:
			return PFG_ETC2_RGB8A1_UNORM_SRGB;
		case PFG_ASTC_RGBA_UNORM_4X4_LDR:
			return PFG_ASTC_RGBA_UNORM_4X4_sRGB;
		case PFG_ASTC_RGBA_UNORM_5X4_LDR:
			return PFG_ASTC_RGBA_UNORM_5X4_sRGB;
		case PFG_ASTC_RGBA_UNORM_5X5_LDR:
			return PFG_ASTC_RGBA_UNORM_5X5_sRGB;
		case PFG_ASTC_RGBA_UNORM_6X5_LDR:
			return PFG_ASTC_RGBA_UNORM_6X5_sRGB;
		case PFG_ASTC_RGBA_UNORM_6X6_LDR:
			return PFG_ASTC_RGBA_UNORM_6X6_sRGB;
		case PFG_ASTC_RGBA_UNORM_8X5_LDR:
			return PFG_ASTC_RGBA_UNORM_8X5_sRGB;
		case PFG_ASTC_RGBA_UNORM_8X6_LDR:
			return PFG_ASTC_RGBA_UNORM_8X6_sRGB;
		case PFG_ASTC_RGBA_UNORM_8X8_LDR:
			return PFG_ASTC_RGBA_UNORM_8X8_sRGB;
		case PFG_ASTC_RGBA_UNORM_10X5_LDR:
			return PFG_ASTC_RGBA_UNORM_10X5_sRGB;
		case PFG_ASTC_RGBA_UNORM_10X6_LDR:
			return PFG_ASTC_RGBA_UNORM_10X6_sRGB;
		case PFG_ASTC_RGBA_UNORM_10X8_LDR:
			return PFG_ASTC_RGBA_UNORM_10X8_sRGB;
		case PFG_ASTC_RGBA_UNORM_10X10_LDR:
			return PFG_ASTC_RGBA_UNORM_10X10_sRGB;
		case PFG_ASTC_RGBA_UNORM_12X10_LDR:
			return PFG_ASTC_RGBA_UNORM_12X10_sRGB;
		case PFG_ASTC_RGBA_UNORM_12X12_LDR:
			return PFG_ASTC_RGBA_UNORM_12X12_sRGB;
		default:
			return format;
	}

	return format;
}
//-----------------------------------------------------------------------------------
PixelFormatGpu PixelFormatGpuUtils::getEquivalentLinear(PixelFormatGpu sRgbFormat) {
	switch (sRgbFormat) {
		case PFG_RGBA8_UNORM_SRGB:
			return PFG_RGBA8_UNORM;
		case PFG_BC1_UNORM_SRGB:
			return PFG_BC1_UNORM;
		case PFG_BC2_UNORM_SRGB:
			return PFG_BC2_UNORM;
		case PFG_BC3_UNORM_SRGB:
			return PFG_BC3_UNORM;
		case PFG_BGRA8_UNORM_SRGB:
			return PFG_BGRA8_UNORM;
		case PFG_BGRX8_UNORM_SRGB:
			return PFG_BGRX8_UNORM;
		case PFG_BC7_UNORM_SRGB:
			return PFG_BC7_UNORM;
		case PFG_RGB8_UNORM_SRGB:
			return PFG_RGB8_UNORM;
		case PFG_BGR8_UNORM_SRGB:
			return PFG_BGR8_UNORM;
		case PFG_ETC2_RGB8_UNORM_SRGB:
			return PFG_ETC2_RGB8_UNORM;
		case PFG_ETC2_RGBA8_UNORM_SRGB:
			return PFG_ETC2_RGBA8_UNORM;
		case PFG_ETC2_RGB8A1_UNORM_SRGB:
			return PFG_ETC2_RGB8A1_UNORM;
		case PFG_ASTC_RGBA_UNORM_4X4_sRGB:
			return PFG_ASTC_RGBA_UNORM_4X4_LDR;
		case PFG_ASTC_RGBA_UNORM_5X4_sRGB:
			return PFG_ASTC_RGBA_UNORM_5X4_LDR;
		case PFG_ASTC_RGBA_UNORM_5X5_sRGB:
			return PFG_ASTC_RGBA_UNORM_5X5_LDR;
		case PFG_ASTC_RGBA_UNORM_6X5_sRGB:
			return PFG_ASTC_RGBA_UNORM_6X5_LDR;
		case PFG_ASTC_RGBA_UNORM_6X6_sRGB:
			return PFG_ASTC_RGBA_UNORM_6X6_LDR;
		case PFG_ASTC_RGBA_UNORM_8X5_sRGB:
			return PFG_ASTC_RGBA_UNORM_8X5_LDR;
		case PFG_ASTC_RGBA_UNORM_8X6_sRGB:
			return PFG_ASTC_RGBA_UNORM_8X6_LDR;
		case PFG_ASTC_RGBA_UNORM_8X8_sRGB:
			return PFG_ASTC_RGBA_UNORM_8X8_LDR;
		case PFG_ASTC_RGBA_UNORM_10X5_sRGB:
			return PFG_ASTC_RGBA_UNORM_10X5_LDR;
		case PFG_ASTC_RGBA_UNORM_10X6_sRGB:
			return PFG_ASTC_RGBA_UNORM_10X6_LDR;
		case PFG_ASTC_RGBA_UNORM_10X8_sRGB:
			return PFG_ASTC_RGBA_UNORM_10X8_LDR;
		case PFG_ASTC_RGBA_UNORM_10X10_sRGB:
			return PFG_ASTC_RGBA_UNORM_10X10_LDR;
		case PFG_ASTC_RGBA_UNORM_12X10_sRGB:
			return PFG_ASTC_RGBA_UNORM_12X10_LDR;
		case PFG_ASTC_RGBA_UNORM_12X12_sRGB:
			return PFG_ASTC_RGBA_UNORM_12X12_LDR;
		default:
			return sRgbFormat;
	}

	return sRgbFormat;
}
//-----------------------------------------------------------------------------------
PixelFormatGpu PixelFormatGpuUtils::getFamily(PixelFormatGpu format) {
	switch (format) {
		case PFG_RGBA32_FLOAT:
		case PFG_RGBA32_UINT:
		case PFG_RGBA32_SINT:
			return PFG_RGBA32_UINT;

		case PFG_RGB32_FLOAT:
		case PFG_RGB32_UINT:
		case PFG_RGB32_SINT:
			return PFG_RGB32_UINT;

		case PFG_RGBA16_FLOAT:
		case PFG_RGBA16_UNORM:
		case PFG_RGBA16_UINT:
		case PFG_RGBA16_SNORM:
		case PFG_RGBA16_SINT:
			return PFG_RGBA16_UINT;

		case PFG_RG32_FLOAT:
		case PFG_RG32_UINT:
		case PFG_RG32_SINT:
			return PFG_RG32_UINT;

		case PFG_R10G10B10A2_UNORM:
		case PFG_R10G10B10A2_UINT:
			return PFG_R10G10B10A2_UINT;

		case PFG_R11G11B10_FLOAT:
			return PFG_R11G11B10_FLOAT;

		case PFG_RGBA8_UNORM:
		case PFG_RGBA8_UNORM_SRGB:
		case PFG_RGBA8_UINT:
		case PFG_RGBA8_SNORM:
		case PFG_RGBA8_SINT:
			return PFG_RGBA8_UNORM;

		case PFG_RG16_FLOAT:
		case PFG_RG16_UNORM:
		case PFG_RG16_UINT:
		case PFG_RG16_SNORM:
		case PFG_RG16_SINT:
			return PFG_RG16_UINT;

		case PFG_D32_FLOAT:
		case PFG_R32_FLOAT:
		case PFG_R32_UINT:
		case PFG_R32_SINT:
			return PFG_R32_UINT;

		case PFG_D24_UNORM:
		case PFG_D24_UNORM_S8_UINT:
			return PFG_D24_UNORM_S8_UINT;

		case PFG_RG8_UNORM:
		case PFG_RG8_UINT:
		case PFG_RG8_SNORM:
		case PFG_RG8_SINT:
			return PFG_RG8_UINT;

		case PFG_R16_FLOAT:
		case PFG_D16_UNORM:
		case PFG_R16_UNORM:
		case PFG_R16_UINT:
		case PFG_R16_SNORM:
		case PFG_R16_SINT:
			return PFG_R16_UINT;

		case PFG_R8_UNORM:
		case PFG_R8_UINT:
		case PFG_R8_SNORM:
		case PFG_R8_SINT:
			return PFG_R8_UINT;

		case PFG_BC1_UNORM:
		case PFG_BC1_UNORM_SRGB:
			return PFG_BC1_UNORM;
		case PFG_BC2_UNORM:
		case PFG_BC2_UNORM_SRGB:
			return PFG_BC2_UNORM;
		case PFG_BC3_UNORM:
		case PFG_BC3_UNORM_SRGB:
			return PFG_BC3_UNORM;
		case PFG_BC4_UNORM:
		case PFG_BC4_SNORM:
			return PFG_BC4_UNORM;
		case PFG_BC5_UNORM:
		case PFG_BC5_SNORM:
			return PFG_BC5_UNORM;

		case PFG_BGRA8_UNORM:
		case PFG_BGRA8_UNORM_SRGB:
			return PFG_BGRA8_UNORM;

		case PFG_BGRX8_UNORM:
		case PFG_BGRX8_UNORM_SRGB:
			return PFG_BGRX8_UNORM;

		case PFG_BC6H_UF16:
		case PFG_BC6H_SF16:
			return PFG_BC6H_UF16;

		case PFG_BC7_UNORM:
		case PFG_BC7_UNORM_SRGB:
			return PFG_BC7_UNORM;

		default:
			return format;
	}

	return format;
}

static const uint32_t PFF_COMPRESSED_COMMON = PixelFormatGpuUtils::PFF_COMPRESSED |
											  PixelFormatGpuUtils::PFF_INTEGER |
											  PixelFormatGpuUtils::PFF_NORMALIZED;

PixelFormatGpuUtils::PixelFormatDesc PixelFormatGpuUtils::msPixelFormatDesc[PFG_COUNT + 1u] = {
	{ "PFG_UNKNOWN", 1u, 0, 0, 0 },
	{ "PFG_NULL", 1u, 0, 0, 0 },
	{ "PFG_RGBA32_FLOAT", 4u, 4u * sizeof(uint32_t), PFL_RGBA32, PFF_FLOAT },
	{ "PFG_RGBA32_UINT", 4u, 4u * sizeof(uint32_t), PFL_RGBA32, PFF_INTEGER },
	{ "PFG_RGBA32_INT", 4u, 4u * sizeof(uint32_t), PFL_RGBA32, PFF_INTEGER | PFF_SIGNED },

	{ "PFG_RGB32_FLOAT", 3u, 3u * sizeof(uint32_t), PFL_RGB32, PFF_FLOAT },
	{ "PFG_RGB32_UINT", 3u, 3u * sizeof(uint32_t), PFL_RGB32, PFF_INTEGER },
	{ "PFG_RGB32_INT", 3u, 3u * sizeof(uint32_t), PFL_RGB32, PFF_INTEGER | PFF_SIGNED },

	{ "PFG_RGBA16_FLOAT", 4u, 4u * sizeof(uint16_t), PFL_RGBA16, PFF_HALF },
	{ "PFG_RGBA16_UNORM", 4u, 4u * sizeof(uint16_t), PFL_RGBA16, PFF_NORMALIZED },
	{ "PFG_RGBA16_UINT", 4u, 4u * sizeof(uint16_t), PFL_RGBA16, PFF_INTEGER },
	{ "PFG_RGBA16_SNORM", 4u, 4u * sizeof(uint16_t), PFL_RGBA16, PFF_NORMALIZED | PFF_SIGNED },
	{ "PFG_RGBA16_SINT", 4u, 4u * sizeof(uint16_t), PFL_RGBA16, PFF_INTEGER | PFF_SIGNED },

	{ "PFG_RG32_FLOAT", 2u, 2u * sizeof(uint32_t), PFL_RG32, PFF_FLOAT },
	{ "PFG_RG32_UINT", 2u, 2u * sizeof(uint32_t), PFL_RG32, PFF_INTEGER },
	{ "PFG_RG32_SINT", 2u, 2u * sizeof(uint32_t), PFL_RG32, PFF_INTEGER | PFF_SIGNED },

	{ "PFG_D32_FLOAT_S8X24_UINT", 2u, 2u * sizeof(uint32_t), PFL_OTHER, PFF_FLOAT | PFF_DEPTH | PFF_STENCIL },

	{ "PFG_R10G10B10A2_UNORM", 4u, 1u * sizeof(uint32_t), PFL_OTHER, PFF_NORMALIZED },
	{ "PFG_R10G10B10A2_UINT", 4u, 1u * sizeof(uint32_t), PFL_OTHER, PFF_INTEGER },
	{ "PFG_R11G11B10_FLOAT", 3u, 1u * sizeof(uint32_t), PFL_OTHER, PFF_FLOAT_RARE },

	{ "PFG_RGBA8_UNORM", 4u, 4u * sizeof(uint8_t), PFL_RGBA8, PFF_NORMALIZED },
	{ "PFG_RGBA8_UNORM_SRGB", 4u, 4u * sizeof(uint8_t), PFL_RGBA8, PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_RGBA8_UINT", 4u, 4u * sizeof(uint8_t), PFL_RGBA8, PFF_INTEGER },
	{ "PFG_RGBA8_SNORM", 4u, 4u * sizeof(uint8_t), PFL_RGBA8, PFF_NORMALIZED | PFF_SIGNED },
	{ "PFG_RGBA8_SINT", 4u, 4u * sizeof(uint8_t), PFL_RGBA8, PFF_INTEGER | PFF_SIGNED },

	{ "PFG_RG16_FLOAT", 2u, 2u * sizeof(uint16_t), PFL_RG16, PFF_HALF },
	{ "PFG_RG16_UNORM", 2u, 2u * sizeof(uint16_t), PFL_RG16, PFF_NORMALIZED },
	{ "PFG_RG16_UINT", 2u, 2u * sizeof(uint16_t), PFL_RG16, PFF_INTEGER },
	{ "PFG_RG16_SNORM", 2u, 2u * sizeof(uint16_t), PFL_RG16, PFF_NORMALIZED | PFF_SIGNED },
	{ "PFG_RG16_SINT", 2u, 2u * sizeof(uint16_t), PFL_RG16, PFF_INTEGER | PFF_SIGNED },

	{ "PFG_D32_FLOAT", 1u, 1u * sizeof(uint32_t), PFL_R32, PFF_FLOAT | PFF_DEPTH },
	{ "PFG_R32_FLOAT", 1u, 1u * sizeof(uint32_t), PFL_R32, PFF_FLOAT },
	{ "PFG_R32_UINT", 1u, 1u * sizeof(uint32_t), PFL_R32, PFF_INTEGER },
	{ "PFG_R32_SINT", 1u, 1u * sizeof(uint32_t), PFL_R32, PFF_INTEGER | PFF_SIGNED },

	{ "PFG_D24_UNORM", 1u, 1u * sizeof(uint32_t), PFL_OTHER, PFF_NORMALIZED | PFF_DEPTH },
	{ "PFG_D24_UNORM_S8_UINT", 1u, 1u * sizeof(uint32_t), PFL_OTHER, PFF_NORMALIZED | PFF_DEPTH | PFF_STENCIL },

	{ "PFG_RG8_UNORM", 2u, 2u * sizeof(uint8_t), PFL_RG8, PFF_NORMALIZED },
	{ "PFG_RG8_UINT", 2u, 2u * sizeof(uint8_t), PFL_RG8, PFF_INTEGER },
	{ "PFG_RG8_SNORM", 2u, 2u * sizeof(uint8_t), PFL_RG8, PFF_NORMALIZED | PFF_SIGNED },
	{ "PFG_RG8_SINT", 2u, 2u * sizeof(uint8_t), PFL_RG8, PFF_INTEGER | PFF_SIGNED },

	{ "PFG_R16_FLOAT", 1u, 1u * sizeof(uint16_t), PFL_R16, PFF_HALF },
	{ "PFG_D16_UNORM", 1u, 1u * sizeof(uint16_t), PFL_R16, PFF_NORMALIZED | PFF_DEPTH },
	{ "PFG_R16_UNORM", 1u, 1u * sizeof(uint16_t), PFL_R16, PFF_NORMALIZED },
	{ "PFG_R16_UINT", 1u, 1u * sizeof(uint16_t), PFL_R16, PFF_INTEGER },
	{ "PFG_R16_SNORM", 1u, 1u * sizeof(uint16_t), PFL_R16, PFF_NORMALIZED | PFF_SIGNED },
	{ "PFG_R16_SINT", 1u, 1u * sizeof(uint16_t), PFL_R16, PFF_INTEGER | PFF_SIGNED },

	{ "PFG_R8_UNORM", 1u, 1u * sizeof(uint8_t), PFL_R8, PFF_NORMALIZED },
	{ "PFG_R8_UINT", 1u, 1u * sizeof(uint8_t), PFL_R8, PFF_INTEGER },
	{ "PFG_R8_SNORM", 1u, 1u * sizeof(uint8_t), PFL_R8, PFF_NORMALIZED | PFF_SIGNED },
	{ "PFG_R8_SINT", 1u, 1u * sizeof(uint8_t), PFL_R8, PFF_INTEGER | PFF_SIGNED },
	{ "PFG_A8_UNORM", 1u, 1u * sizeof(uint8_t), PFL_R8, PFF_NORMALIZED },
	{ "PFG_R1_UNORM", 1u, 0, 0, 0 }, // ???

	{ "PFG_R9G9B9E5_SHAREDEXP", 1u, 1u * sizeof(uint32_t), PFL_OTHER, PFF_FLOAT_RARE },

	{ "PFG_R8G8_B8G8_UNORM", 4u, 4u * sizeof(uint8_t), PFL_OTHER, PFF_NORMALIZED },
	{ "PFG_G8R8_G8B8_UNORM", 4u, 4u * sizeof(uint8_t), PFL_OTHER, PFF_NORMALIZED | PFF_SIGNED },

	{ "PFG_BC1_UNORM", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_BC1_UNORM_SRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SRGB },

	{ "PFG_BC2_UNORM", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_BC2_UNORM_SRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SRGB },

	{ "PFG_BC3_UNORM", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_BC3_UNORM_SRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SRGB },

	{ "PFG_BC4_UNORM", 1u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_BC4_SNORM", 1u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SIGNED },

	{ "PFG_BC5_UNORM", 2u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_BC5_SNORM", 2u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SIGNED },

	{ "PFG_B5G6R5_UNORM", 3u, 1u * sizeof(uint16_t), PFL_OTHER, PFF_NORMALIZED },
	{ "PFG_B5G5R5A1_UNORM", 3u, 1u * sizeof(uint16_t), PFL_OTHER, PFF_NORMALIZED },
	{ "PFG_BGRA8_UNORM", 4u, 4u * sizeof(uint8_t), PFL_BGRA8, PFF_NORMALIZED },
	{ "PFG_BGRX8_UNORM", 3u, 4u * sizeof(uint8_t), PFL_BGRX8, PFF_NORMALIZED },
	{ "PFG_R10G10B10_XR_BIAS_A2_UNORM", 4u, 1u * sizeof(uint32_t), PFL_OTHER, PFF_FLOAT_RARE },

	{ "PFG_BGRA8_UNORM_SRGB", 4u, 4u * sizeof(uint8_t), PFL_BGRA8, PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_BGRX8_UNORM_SRGB", 3u, 4u * sizeof(uint8_t), PFL_BGRX8, PFF_NORMALIZED | PFF_SRGB },

	{ "PFG_BC6H_UF16", 3u, 0, PFL_OTHER, PFF_COMPRESSED | PFF_FLOAT_RARE },
	{ "PFG_BC6H_SF16", 3u, 0, PFL_OTHER, PFF_COMPRESSED | PFF_FLOAT_RARE | PFF_SIGNED },

	{ "PFG_BC7_UNORM", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_BC7_UNORM_SRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SRGB },

	{ "PFG_AYUV", 3u, 0, 0, 0 },
	{ "PFG_Y410", 3u, 0, 0, 0 },
	{ "PFG_Y416", 3u, 0, 0, 0 },
	{ "PFG_NV12", 3u, 0, 0, 0 },
	{ "PFG_P010", 3u, 0, 0, 0 },
	{ "PFG_P016", 3u, 0, 0, 0 },
	{ "PFG_420_OPAQUE", 3u, 0, 0, 0 },
	{ "PFG_YUY2", 3u, 0, 0, 0 },
	{ "PFG_Y210", 3u, 0, 0, 0 },
	{ "PFG_Y216", 3u, 0, 0, 0 },
	{ "PFG_NV11", 3u, 0, 0, 0 },
	{ "PFG_AI44", 3u, 0, 0, 0 },
	{ "PFG_IA44", 3u, 0, 0, 0 },
	{ "PFG_P8", 1u, 1u * sizeof(uint8_t), PFL_OTHER, PFF_PALLETE },
	{ "PFG_A8P8", 2u, 2u * sizeof(uint8_t), PFL_OTHER, PFF_PALLETE },
	{ "PFG_B4G4R4A4_UNORM", 4u, 1u * sizeof(uint16_t), PFL_OTHER, PFF_NORMALIZED },
	{ "PFG_P208", 3u, 0, 0, 0 },
	{ "PFG_V208", 3u, 0, 0, 0 },
	{ "PFG_V408", 3u, 0, 0, 0 },

	{ "PFG_RGB8_UNORM", 3u, 3u * sizeof(uint8_t), PFL_RGB8, PFF_NORMALIZED },
	{ "PFG_RGB8_UNORM_SRGB", 3u, 3u * sizeof(uint8_t), PFL_RGB8, PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_BGR8_UNORM", 3u, 3u * sizeof(uint8_t), PFL_BGR8, PFF_NORMALIZED },
	{ "PFG_BGR8_UNORM_SRGB", 3u, 3u * sizeof(uint8_t), PFL_BGR8, PFF_NORMALIZED | PFF_SRGB },

	{ "PFG_RGB16_UNORM", 3u, 3u * sizeof(uint16_t), PFL_RGB16, PFF_NORMALIZED },

	{ "PFG_PVRTC_RGB2", 3u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_PVRTC_RGB2_SRGB", 3u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SRGB },
	{ "PFG_PVRTC_RGBA2", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_PVRTC_RGBA2_SRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SRGB },
	{ "PFG_PVRTC_RGB4", 3u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_PVRTC_RGB4_SRGB", 3u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SRGB },
	{ "PFG_PVRTC_RGBA4", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_PVRTC_RGBA4_SRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SRGB },
	{ "PFG_PVRTC2_2BPP", 3u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_PVRTC2_2BPP_SRGB", 3u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SRGB },
	{ "PFG_PVRTC2_4BPP", 3u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_PVRTC2_4BPP_SRGB", 3u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SRGB },

	{ "PFG_ETC1_RGB8_UNORM", 3u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_ETC2_RGB8_UNORM", 3u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_ETC2_RGB8_UNORM_SRGB", 3u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SRGB },
	{ "PFG_ETC2_RGBA8_UNORM", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_ETC2_RGBA8_UNORM_SRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SRGB },
	{ "PFG_ETC2_RGB8A1_UNORM", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_ETC2_RGB8A1_UNORM_SRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SRGB },
	{ "PFG_EAC_R11_UNORM", 1u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_EAC_R11_SNORM", 1u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SIGNED },
	{ "PFG_EAC_R11G11_UNORM", 2u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_EAC_R11G11_SNORM", 2u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_SIGNED },

	{ "PFG_ATC_RGB", 3u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_ATC_RGBA_EXPLICIT_ALPHA", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },
	{ "PFG_ATC_RGBA_INTERPOLATED_ALPHA", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON },

	{ "PFG_ASTC_RGBA_UNORM_4X4_LDR", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED },
	{ "PFG_ASTC_RGBA_UNORM_5X4_LDR", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED },
	{ "PFG_ASTC_RGBA_UNORM_5X5_LDR", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED },
	{ "PFG_ASTC_RGBA_UNORM_6X5_LDR", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED },
	{ "PFG_ASTC_RGBA_UNORM_6X6_LDR", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED },
	{ "PFG_ASTC_RGBA_UNORM_8X5_LDR", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED },
	{ "PFG_ASTC_RGBA_UNORM_8X6_LDR", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED },
	{ "PFG_ASTC_RGBA_UNORM_6X5_LDR", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED },
	{ "PFG_ASTC_RGBA_UNORM_10X5_LDR", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED },
	{ "PFG_ASTC_RGBA_UNORM_10X6_LDR", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED },
	{ "PFG_ASTC_RGBA_UNORM_10X8_LDR", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED },
	{ "PFG_ASTC_RGBA_UNORM_10X10_LDR", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED },
	{ "PFG_ASTC_RGBA_UNORM_12X10_LDR", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED },
	{ "PFG_ASTC_RGBA_UNORM_12X12_LDR", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED },

	{ "PFG_ASTC_RGBA_UNORM_4X4_sRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_ASTC_RGBA_UNORM_5X4_sRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_ASTC_RGBA_UNORM_5X5_sRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_ASTC_RGBA_UNORM_6X5_sRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_ASTC_RGBA_UNORM_6X6_sRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_ASTC_RGBA_UNORM_8X5_sRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_ASTC_RGBA_UNORM_8X6_sRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_ASTC_RGBA_UNORM_6X5_sRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_ASTC_RGBA_UNORM_10X5_sRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_ASTC_RGBA_UNORM_10X6_sRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_ASTC_RGBA_UNORM_10X8_sRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_ASTC_RGBA_UNORM_10X10_sRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_ASTC_RGBA_UNORM_12X10_sRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED | PFF_SRGB },
	{ "PFG_ASTC_RGBA_UNORM_12X12_sRGB", 4u, 0, PFL_OTHER, PFF_COMPRESSED_COMMON | PFF_NORMALIZED | PFF_SRGB },

	{ "PFG_COUNT", 1u, 0, 0 },
};