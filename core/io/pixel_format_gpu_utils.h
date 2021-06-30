/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#ifndef _OgrePixelFormatGpuUtils_H_
#define _OgrePixelFormatGpuUtils_H_

#include "core/io/resource.h"
#include "core/math/color.h"
#include "core/math/rect2.h"

/** \addtogroup Core
    *  @{
    */
/** \addtogroup Image
    *  @{
    */
/** The pixel format used for images, textures, and render surfaces */
enum PixelFormatGpu {
	PFG_UNKNOWN,
	PFG_NULL,

	// Starting Here, start D3D11 formats (it isn't 1:1 with DXGI_FORMAT_* though)

	PFG_RGBA32_FLOAT,
	PFG_RGBA32_UINT,
	PFG_RGBA32_SINT,

	PFG_RGB32_FLOAT,
	PFG_RGB32_UINT,
	PFG_RGB32_SINT,

	PFG_RGBA16_FLOAT,
	PFG_RGBA16_UNORM,
	PFG_RGBA16_UINT,
	PFG_RGBA16_SNORM,
	PFG_RGBA16_SINT,

	PFG_RG32_FLOAT,
	PFG_RG32_UINT,
	PFG_RG32_SINT,

	PFG_D32_FLOAT_S8X24_UINT,

	PFG_R10G10B10A2_UNORM,
	PFG_R10G10B10A2_UINT,
	PFG_R11G11B10_FLOAT,

	PFG_RGBA8_UNORM,
	PFG_RGBA8_UNORM_SRGB,
	PFG_RGBA8_UINT,
	PFG_RGBA8_SNORM,
	PFG_RGBA8_SINT,

	PFG_RG16_FLOAT,
	PFG_RG16_UNORM,
	PFG_RG16_UINT,
	PFG_RG16_SNORM,
	PFG_RG16_SINT,

	PFG_D32_FLOAT,
	PFG_R32_FLOAT,
	PFG_R32_UINT,
	PFG_R32_SINT,

	PFG_D24_UNORM,
	PFG_D24_UNORM_S8_UINT,

	PFG_RG8_UNORM,
	PFG_RG8_UINT,
	PFG_RG8_SNORM,
	PFG_RG8_SINT,

	PFG_R16_FLOAT,
	PFG_D16_UNORM,
	PFG_R16_UNORM,
	PFG_R16_UINT,
	PFG_R16_SNORM,
	PFG_R16_SINT,

	PFG_R8_UNORM,
	PFG_R8_UINT,
	PFG_R8_SNORM,
	PFG_R8_SINT,
	PFG_A8_UNORM,
	PFG_R1_UNORM,
	PFG_R9G9B9E5_SHAREDEXP,
	/// D3D11 only. A four-component, 32-bit unsigned-normalized-integer format.
	/// This packed RGB format is analogous to the UYVY format.
	/// Each 32-bit block describes a pair of pixels: (R8, G8, B8) and (R8, G8, B8)
	/// where the R8/B8 values are repeated, and the G8 values are unique to each pixel.
	PFG_R8G8_B8G8_UNORM,
	/// D3D11 only. See PFG_R8G8_B8G8_UNORM
	PFG_G8R8_G8B8_UNORM,

	/// BC1, aka DXT1 & DXT2
	PFG_BC1_UNORM,
	PFG_BC1_UNORM_SRGB,

	/// BC2, aka DXT3 & DXT4
	PFG_BC2_UNORM,
	PFG_BC2_UNORM_SRGB,

	/// BC3, aka DXT5
	PFG_BC3_UNORM,
	PFG_BC3_UNORM_SRGB,

	/// One channel compressed 4bpp. Ideal for greyscale.
	PFG_BC4_UNORM,
	/// Two channels compressed 8bpp. Ideal for normal maps or greyscale + alpha.
	PFG_BC4_SNORM,

	PFG_BC5_UNORM,
	PFG_BC5_SNORM,

	PFG_B5G6R5_UNORM,
	PFG_B5G5R5A1_UNORM,
	/// Avoid this one (prefer RGBA8).
	PFG_BGRA8_UNORM,
	/// Avoid this one (prefer RGBA8).
	PFG_BGRX8_UNORM,
	PFG_R10G10B10_XR_BIAS_A2_UNORM,

	/// Avoid this one (prefer RGBA8).
	PFG_BGRA8_UNORM_SRGB,
	/// Avoid this one (prefer RGBA8).
	PFG_BGRX8_UNORM_SRGB,

	/// BC6H format (unsigned 16 bit float)
	PFG_BC6H_UF16,
	/// BC6H format (signed 16 bit float)
	PFG_BC6H_SF16,

	PFG_BC7_UNORM,
	PFG_BC7_UNORM_SRGB,

	PFG_AYUV,
	PFG_Y410,
	PFG_Y416,
	PFG_NV12,
	PFG_P010,
	PFG_P016,
	PFG_420_OPAQUE,
	PFG_YUY2,
	PFG_Y210,
	PFG_Y216,
	PFG_NV11,
	PFG_AI44,
	PFG_IA44,
	PFG_P8,
	PFG_A8P8,
	PFG_B4G4R4A4_UNORM,
	PFG_P208,
	PFG_V208,
	PFG_V408,

	// Here ends D3D11 formats (it isn't 1:1 with DXGI_FORMAT_* though)

	/// 24bpp storage formats, CPU only.
	PFG_RGB8_UNORM,
	PFG_RGB8_UNORM_SRGB,
	PFG_BGR8_UNORM,
	PFG_BGR8_UNORM_SRGB,

	/// 48bpp storage formats, CPU only.
	PFG_RGB16_UNORM,

	/// PVRTC (PowerVR) RGB 2 bpp
	PFG_PVRTC_RGB2,
	PFG_PVRTC_RGB2_SRGB,
	/// PVRTC (PowerVR) RGBA 2 bpp
	PFG_PVRTC_RGBA2,
	PFG_PVRTC_RGBA2_SRGB,
	/// PVRTC (PowerVR) RGB 4 bpp
	PFG_PVRTC_RGB4,
	PFG_PVRTC_RGB4_SRGB,
	/// PVRTC (PowerVR) RGBA 4 bpp
	PFG_PVRTC_RGBA4,
	PFG_PVRTC_RGBA4_SRGB,
	/// PVRTC (PowerVR) Version 2, 2 bpp
	PFG_PVRTC2_2BPP,
	PFG_PVRTC2_2BPP_SRGB,
	/// PVRTC (PowerVR) Version 2, 4 bpp
	PFG_PVRTC2_4BPP,
	PFG_PVRTC2_4BPP_SRGB,

	/// ETC1 (Ericsson Texture Compression)
	PFG_ETC1_RGB8_UNORM,
	/// ETC2 (Ericsson Texture Compression). Mandatory in GLES 3.0
	PFG_ETC2_RGB8_UNORM,
	PFG_ETC2_RGB8_UNORM_SRGB,
	PFG_ETC2_RGBA8_UNORM,
	PFG_ETC2_RGBA8_UNORM_SRGB,
	PFG_ETC2_RGB8A1_UNORM,
	PFG_ETC2_RGB8A1_UNORM_SRGB,
	/// EAC compression (built on top of ETC2) Mandatory in GLES 3.0 for 1 channel & 2 channels
	PFG_EAC_R11_UNORM,
	PFG_EAC_R11_SNORM,
	PFG_EAC_R11G11_UNORM,
	PFG_EAC_R11G11_SNORM,

	/// ATC (AMD_compressed_ATC_texture)
	PFG_ATC_RGB,
	/// ATC (AMD_compressed_ATC_texture)
	PFG_ATC_RGBA_EXPLICIT_ALPHA,
	/// ATC (AMD_compressed_ATC_texture)
	PFG_ATC_RGBA_INTERPOLATED_ALPHA,

	/// ASTC (ARM Adaptive Scalable Texture Compression RGBA, block size 4x4)
	PFG_ASTC_RGBA_UNORM_4X4_LDR,
	PFG_ASTC_RGBA_UNORM_5X4_LDR,
	PFG_ASTC_RGBA_UNORM_5X5_LDR,
	PFG_ASTC_RGBA_UNORM_6X5_LDR,
	PFG_ASTC_RGBA_UNORM_6X6_LDR,
	PFG_ASTC_RGBA_UNORM_8X5_LDR,
	PFG_ASTC_RGBA_UNORM_8X6_LDR,
	PFG_ASTC_RGBA_UNORM_8X8_LDR,
	PFG_ASTC_RGBA_UNORM_10X5_LDR,
	PFG_ASTC_RGBA_UNORM_10X6_LDR,
	PFG_ASTC_RGBA_UNORM_10X8_LDR,
	PFG_ASTC_RGBA_UNORM_10X10_LDR,
	PFG_ASTC_RGBA_UNORM_12X10_LDR,
	PFG_ASTC_RGBA_UNORM_12X12_LDR,

	/// ASTC (ARM Adaptive Scalable Texture Compression RGBA_UNORM sRGB, block size 4x4)
	PFG_ASTC_RGBA_UNORM_4X4_sRGB,
	PFG_ASTC_RGBA_UNORM_5X4_sRGB,
	PFG_ASTC_RGBA_UNORM_5X5_sRGB,
	PFG_ASTC_RGBA_UNORM_6X5_sRGB,
	PFG_ASTC_RGBA_UNORM_6X6_sRGB,
	PFG_ASTC_RGBA_UNORM_8X5_sRGB,
	PFG_ASTC_RGBA_UNORM_8X6_sRGB,
	PFG_ASTC_RGBA_UNORM_8X8_sRGB,
	PFG_ASTC_RGBA_UNORM_10X5_sRGB,
	PFG_ASTC_RGBA_UNORM_10X6_sRGB,
	PFG_ASTC_RGBA_UNORM_10X8_sRGB,
	PFG_ASTC_RGBA_UNORM_10X10_sRGB,
	PFG_ASTC_RGBA_UNORM_12X10_sRGB,
	PFG_ASTC_RGBA_UNORM_12X12_sRGB,

	PFG_COUNT
};
struct TextureBox {
	uint32_t x, y, z, sliceStart;
	uint32_t width, height, depth, numSlices;
	/// When TextureBox contains a compressed format, bytesPerPixel contains
	/// the pixel format instead. See getCompressedPixelFormat.
	uint32_t bytesPerPixel;
	uint32_t bytesPerRow;
	uint32_t bytesPerImage;
	/// Pointer is never owned by us. Do not alter where
	/// data points to (e.g. do not increment it)
	void *data;

	TextureBox() :
			x(0), y(0), z(0), sliceStart(0), width(0), height(0), depth(0), numSlices(0), bytesPerPixel(0), bytesPerRow(0), bytesPerImage(0), data(0) {
	}

	TextureBox(uint32_t _width, uint32_t _height, uint32_t _depth, uint32_t _numSlices,
			uint32_t _bytesPerPixel, uint32_t _bytesPerRow, uint32_t _bytesPerImage) :
			x(0), y(0), z(0), sliceStart(0), width(_width), height(_height), depth(_depth), numSlices(_numSlices), bytesPerPixel(_bytesPerPixel), bytesPerRow(_bytesPerRow), bytesPerImage(_bytesPerImage), data(0) {
	}

	uint32_t getMaxX(void) const { return x + width; }
	uint32_t getMaxY(void) const { return y + height; }
	uint32_t getMaxZ(void) const { return z + depth; }
	uint32_t getMaxSlice(void) const { return sliceStart + numSlices; }
	uint32_t getDepthOrSlices(void) const { return std::max(depth, numSlices); }
	uint32_t getZOrSlice(void) const { return std::max(z, sliceStart); }

	uint32_t getSizeBytes(void) const { return bytesPerImage * std::max(depth, numSlices); }

	void setCompressedPixelFormat(PixelFormatGpu pixelFormat);
	PixelFormatGpu getCompressedPixelFormat(void) const {
		if (bytesPerPixel < 0xF0000000)
			return PFG_UNKNOWN;
		return static_cast<PixelFormatGpu>(bytesPerPixel - 0xF0000000);
	}

	bool isCompressed(void) const {
		return bytesPerPixel >= 0xF0000000;
	}

	/// Returns true if 'other' & 'this' have the same dimensions.
	bool equalSize(const TextureBox &other) const {
		return this->width == other.width &&
			   this->height == other.height &&
			   this->depth == other.depth &&
			   this->numSlices == other.numSlices;
	}

	/// Returns true if 'other' fits inside 'this' (fully, not partially)
	bool fullyContains(const TextureBox &other) const {
		return other.x >= this->x && other.getMaxX() <= this->getMaxX() &&
			   other.y >= this->y && other.getMaxY() <= this->getMaxY() &&
			   other.z >= this->z && other.getMaxZ() <= this->getMaxZ() &&
			   other.sliceStart >= this->sliceStart && other.getMaxSlice() <= this->getMaxSlice();
	}

	/// Returns true if 'this' and 'other' are in partial or full collision.
	bool overlaps(const TextureBox &other) const {
		return !(other.x >= this->getMaxX() ||
				 other.y >= this->getMaxY() ||
				 other.z >= this->getMaxZ() ||
				 other.sliceStart >= this->getMaxSlice() ||
				 other.getMaxX() <= this->x ||
				 other.getMaxY() <= this->y ||
				 other.getMaxZ() <= this->z ||
				 other.getMaxSlice() <= this->sliceStart);
	}

	/// x, y & z are in pixels. Only works for non-compressed formats.
	/// It can work for compressed formats if xPos & yPos are 0.
	void *at(uint32_t xPos, uint32_t yPos, uint32_t zPos) const;

	void *atFromOffsettedOrigin(uint32_t xPos, uint32_t yPos, uint32_t zPos) const {
		return at(xPos + x, yPos + y, zPos + getZOrSlice());
	}

	/// Returns true if this TextureBox does not represent a contiguous region of a
	/// single slice of full texture, and is instead a 2D subregion of a larger texture.
	bool isSubtextureRegion(void) const;
	void copyFrom(const TextureBox &src);
	void copyFrom(void *srcData, uint32_t _width, uint32_t _height, uint32_t _bytesPerRow) {
		TextureBox box(_width, _height, 1u, 1u, 0, _bytesPerRow, _bytesPerRow * _height);
		box.data = srcData;
		copyFrom(box);
	}

	/// Get colour value from a certain location in the image.
	Color getColourAt(uint32_t _x, uint32_t _y, uint32_t _z, PixelFormatGpu pixelFormat) const;

	/// Set colour value at a certain location in the image.
	void setColourAt(const Color &cv, uint32_t _x, uint32_t _y, uint32_t _z,
			PixelFormatGpu pixelFormat);
};
class PixelFormatGpuUtils {
public:
	static inline uint32_t alignToNextMultiple(uint32_t offset, uint32_t alignment) {
		return ((offset + alignment - 1u) / alignment) * alignment;
	}
	static inline uint32_t alignToPreviousMult(uint32_t offset, uint32_t alignment) {
		return (offset / alignment) * alignment;
	}
	/// Pixel components size and order, typeless.
	enum PixelFormatLayout {
		PFL_OTHER = 0,

		PFL_RGBA32,
		PFL_RGB32,
		PFL_RG32,
		PFL_R32,

		PFL_RGBA16,
		PFL_RGB16,
		PFL_RG16,
		PFL_R16,

		PFL_RGBA8,
		PFL_BGRA8,
		PFL_BGRX8,
		PFL_RGB8,
		PFL_BGR8,
		PFL_RG8,
		PFL_R8,

		PFL_COUNT
	};

	enum PixelFormatFlags {
		/// Pixel Format is an actual float (32-bit float)
		PFF_FLOAT = 1u << 0u,
		/// Pixel Format is 16-bit float
		PFF_HALF = 1u << 1u,
		/// Pixel Format is float, but is neither 32-bit nor 16-bit
		/// (some weird one, could have shared exponent or not)
		PFF_FLOAT_RARE = 1u << 2u,
		/// Pixel Format is (signed/unsigned) integer. May be
		/// normalized if PFF_NORMALIZED is present.
		PFF_INTEGER = 1u << 3u,
		/// Pixel Format is either in range [-1; 1] or [0; 1]
		PFF_NORMALIZED = 1u << 4u,
		/// Pixel Format can only be positive or negative.
		/// It's implicit for float/half formats. Lack
		/// of this flag means it's unsigned.
		PFF_SIGNED = 1u << 5u,
		/// This is a depth format. Can be combined with
		/// PFF_FLOAT/PFF_HALF/PFF_INTEGER/PFF_NORMALIZED to get
		/// more info about the depth.
		PFF_DEPTH = 1u << 6u,
		/// This format has stencil.
		PFF_STENCIL = 1u << 7u,
		/// Format is in sRGB space.
		PFF_SRGB = 1u << 8u,
		/// Format is compressed
		PFF_COMPRESSED = 1u << 9u,
		/// Format is palletized
		PFF_PALLETE = 1u << 10u
	};

protected:
	struct PixelFormatDesc {
		const char *name;
		uint8_t components;
		uint8_t bytesPerPixel;
		uint16_t layout;
		uint32_t flags;
	};

	static PixelFormatDesc msPixelFormatDesc[PFG_COUNT + 1u];

	static inline const PixelFormatDesc &getDescriptionFor(const PixelFormatGpu fmt);

	template <typename T>
	static bool convertFromFloat(const float *rgbaPtr, void *dstPtr,
			uint32_t numComponents, uint32_t flags);
	template <typename T>
	static bool convertToFloat(float *rgbaPtr, const void *srcPtr,
			uint32_t numComponents, uint32_t flags);

public:
	static uint32_t getBytesPerPixel(PixelFormatGpu format);
	static uint32_t getNumberOfComponents(PixelFormatGpu format);
	static PixelFormatLayout getPixelLayout(PixelFormatGpu format);

	static uint32_t getSizeBytes(uint32_t width, uint32_t height, uint32_t depth,
			uint32_t slices, PixelFormatGpu format,
			uint32_t rowAlignment = 1u);

	static uint32_t calculateSizeBytes(uint32_t width, uint32_t height, uint32_t depth,
			uint32_t slices, PixelFormatGpu format,
			uint8_t numMipmaps, uint32_t rowAlignment = 1u);

	/** Returns the maximum number of mipmaps given the resolution
            e.g. at 4x4 there's 3 mipmaps. At 1x1 there's 1 mipmaps.
        @note
            Can return 0 if maxResolution = 0.
        @return
            Mip count.
        */
	static uint8_t getMaxMipmapCount(uint32_t maxResolution);
	static uint8_t getMaxMipmapCount(uint32_t width, uint32_t height);
	static uint8_t getMaxMipmapCount(uint32_t width, uint32_t height, uint32_t depth);

	/// For SW mipmaps, see Image2::supportsSwMipmaps
	static bool supportsHwMipmaps(PixelFormatGpu format);

	/** Returns the minimum width for block compressed schemes. ie. DXT1 compresses in blocks
            of 4x4 pixels. A texture with a width of 2 is just padded to 4.
            When building UV atlases composed of already compressed data being stitched together,
            the block size is very important to know as the resolution of the individual textures
            must be a multiple of this size.
         @remarks
            If the format is not compressed, returns 1.
         @par
            The function can return a value of 0 (as happens with PVRTC & ETC1 compression); this is
            because although they may compress in blocks (i.e. PVRTC uses a 4x4 or 8x4 block), this
            information is useless as the compression scheme doesn't have isolated blocks (modifying
            a single pixel can change the binary data of the entire stream) making it useless for
            subimage sampling or creating UV atlas.
         @param format
            The format to query for. Can be compressed or not.
         @param apiStrict
            When true, obeys the rules of most APIs (i.e. ETC1 can't update subregions according to
            GLES specs). When false, becomes more practical if manipulating by hand (i.e. ETC1's
            subregions can be updated just fine by @bulkCompressedSubregion)
         @return
            The width of compression block, in pixels. Can be 0 (see remarks). If format is not
            compressed, returns 1.
        */
	static uint32_t getCompressedBlockWidth(PixelFormatGpu format, bool apiStrict = true);

	/// See getCompressedBlockWidth
	static uint32_t getCompressedBlockHeight(PixelFormatGpu format, bool apiStrict = true);

	/// Returns in bytes, the size of the compressed block
	static uint32_t getCompressedBlockSize(PixelFormatGpu format);

	static const char *toString(PixelFormatGpu format);

	/** Makes a O(N) search to return the PixelFormatGpu based on its string version.
            Opposite version of toString
        @param name
            Name of the pixel format. e.g. PFG_RGBA8_UNORM_SRGB
        @param exclusionFlags
            Use PixelFormatFlags to exclude certain formats. For example if you don't want
            compressed and depth formats to be returned, pass PFF_COMPRESSED|PFF_DEPTH
        @return
            The format you're looking for, PFG_UNKNOWN if not found.
        */
	static PixelFormatGpu getFormatFromName(const char *name, uint32_t exclusionFlags = 0);
	static PixelFormatGpu getFormatFromName(const String &name, uint32_t exclusionFlags = 0);

	/// Takes an image allocated for GPU usage (i.e. rowAlignment = 4u) from the beginning of
	/// its base mip level 0, and returns a pointer at the beginning of the specified mipLevel.
	static void *advancePointerToMip(void *basePtr, uint32_t width, uint32_t height,
			uint32_t depth, uint32_t numSlices, uint8_t mipLevel,
			PixelFormatGpu format);

	static float toSRGB(float x);
	static float fromSRGB(float x);

	static bool packColour(const float *rgbaPtr, PixelFormatGpu pf, void *dstPtr);
	static bool unpackColour(float *rgbaPtr, PixelFormatGpu pf, const void *srcPtr);
	static void packColour(const Color &rgbaPtr, PixelFormatGpu pf, void *dstPtr);
	static void unpackColour(Color *rgbaPtr, PixelFormatGpu pf, const void *srcPtr);
	static void convertForNormalMapping(TextureBox src, PixelFormatGpu srcFormat,
			TextureBox dst, PixelFormatGpu dstFormat);
	static void bulkPixelConversion(const TextureBox &src, PixelFormatGpu srcFormat,
			TextureBox &dst, PixelFormatGpu dstFormat,
			bool verticalFlip = false);
	/// See PixelFormatFlags
	static uint32_t getFlags(PixelFormatGpu format);
	static bool isFloat(PixelFormatGpu format);
	static bool isHalf(PixelFormatGpu format);
	static bool isFloatRare(PixelFormatGpu format);
	static bool isInteger(PixelFormatGpu format);
	static bool isNormalized(PixelFormatGpu format);
	static bool isSigned(PixelFormatGpu format);
	static bool isDepth(PixelFormatGpu format);
	static bool isStencil(PixelFormatGpu format);
	static bool isSRgb(PixelFormatGpu format);
	static bool isCompressed(PixelFormatGpu format);
	static bool isPallete(PixelFormatGpu format);
	static bool isAccessible(PixelFormatGpu format);
	static bool hasAlpha(PixelFormatGpu format);

	static bool hasSRGBEquivalent(PixelFormatGpu format);
	static PixelFormatGpu getEquivalentSRGB(PixelFormatGpu format);
	static PixelFormatGpu getEquivalentLinear(PixelFormatGpu sRgbFormat);

	static PixelFormatGpu getFamily(PixelFormatGpu format);
};

#endif
