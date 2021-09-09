// SPDX-License-Identifier: Apache-2.0
// ----------------------------------------------------------------------------
// Copyright 2021 Arm Limited
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at:
//
//	 http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
// ----------------------------------------------------------------------------

/**
 * @brief Intrinsics for Armv7 NEON.
 *
 * This module implements a few Armv7-compatible intrinsics indentical to Armv8
 * ones. Thus, astcenc can be compiled using Armv7 architecture.
 */

#ifndef ASTC_VECMATHLIB_NEON_ARMV7_4_H_INCLUDED
#define ASTC_VECMATHLIB_NEON_ARMV7_4_H_INCLUDED

#ifndef ASTCENC_SIMD_INLINE
#error "Include astcenc_vecmathlib.h, do not include directly"
#endif

#include <algorithm>
#include <cfenv>

// arm-linux-gnueabi-gcc contains the following functions by using
// #pragma GCC target ("fpu=neon-fp-armv8"), while clang does not.
#if defined(__clang__)

/**
 * @brief Return the max vector of two vectors.
 *
 * If one vector element is numeric and the other is a quiet NaN,
 * the result placed in the vector is the numerical value.
 */
ASTCENC_SIMD_INLINE float32x4_t vmaxnmq_f32(float32x4_t a, float32x4_t b) {
	uint32x4_t amask = vceqq_f32(a, a);
	uint32x4_t bmask = vceqq_f32(b, b);
	a = vbslq_f32(amask, a, b);
	b = vbslq_f32(bmask, b, a);
	return vmaxq_f32(a, b);
}

/**
 * @brief Return the min vector of two vectors.
 *
 * If one vector element is numeric and the other is a quiet NaN,
 * the result placed in the vector is the numerical value.
 */
ASTCENC_SIMD_INLINE float32x4_t vminnmq_f32(float32x4_t a, float32x4_t b) {
	uint32x4_t amask = vceqq_f32(a, a);
	uint32x4_t bmask = vceqq_f32(b, b);
	a = vbslq_f32(amask, a, b);
	b = vbslq_f32(bmask, b, a);
	return vminq_f32(a, b);
}

/**
 * @brief Return a float rounded to the nearest integer value.
 */
ASTCENC_SIMD_INLINE float32x4_t vrndnq_f32(float32x4_t a) {
	assert(std::fegetround() == FE_TONEAREST);
	float a0 = std::nearbyintf(vgetq_lane_f32(a, 0));
	float a1 = std::nearbyintf(vgetq_lane_f32(a, 1));
	float a2 = std::nearbyintf(vgetq_lane_f32(a, 2));
	float a3 = std::nearbyintf(vgetq_lane_f32(a, 3));
	float32x4_t c{ a0, a1, a2, a3 };
	return c;
}

#endif

/**
 * @brief Return the horizontal maximum of a vector.
 */
ASTCENC_SIMD_INLINE float vmaxvq_f32(float32x4_t a) {
	float a0 = vgetq_lane_f32(a, 0);
	float a1 = vgetq_lane_f32(a, 1);
	float a2 = vgetq_lane_f32(a, 2);
	float a3 = vgetq_lane_f32(a, 3);
	return std::max(std::max(a0, a1), std::max(a2, a3));
}

/**
 * @brief Return the horizontal maximum of a vector.
 */
ASTCENC_SIMD_INLINE float vminvq_f32(float32x4_t a) {
	float a0 = vgetq_lane_f32(a, 0);
	float a1 = vgetq_lane_f32(a, 1);
	float a2 = vgetq_lane_f32(a, 2);
	float a3 = vgetq_lane_f32(a, 3);
	return std::min(std::min(a0, a1), std::min(a2, a3));
}

/**
 * @brief Return the horizontal maximum of a vector.
 */
ASTCENC_SIMD_INLINE int32_t vmaxvq_s32(int32x4_t a) {
	int32_t a0 = vgetq_lane_s32(a, 0);
	int32_t a1 = vgetq_lane_s32(a, 1);
	int32_t a2 = vgetq_lane_s32(a, 2);
	int32_t a3 = vgetq_lane_s32(a, 3);
	return std::max(std::max(a0, a1), std::max(a2, a3));
}

/**
 * @brief Return the horizontal maximum of a vector.
 */
ASTCENC_SIMD_INLINE int32_t vminvq_s32(int32x4_t a) {
	int32_t a0 = vgetq_lane_s32(a, 0);
	int32_t a1 = vgetq_lane_s32(a, 1);
	int32_t a2 = vgetq_lane_s32(a, 2);
	int32_t a3 = vgetq_lane_s32(a, 3);
	return std::min(std::min(a0, a1), std::min(a2, a3));
}

/**
 * @brief Return the sqrt of the lanes in the vector.
 */
ASTCENC_SIMD_INLINE float32x4_t vsqrtq_f32(float32x4_t a) {
	float a0 = std::sqrt(vgetq_lane_f32(a, 0));
	float a1 = std::sqrt(vgetq_lane_f32(a, 1));
	float a2 = std::sqrt(vgetq_lane_f32(a, 2));
	float a3 = std::sqrt(vgetq_lane_f32(a, 3));
	float32x4_t c{ a0, a1, a2, a3 };
	return c;
}

/**
 * @brief Vector by vector division.
 */
ASTCENC_SIMD_INLINE float32x4_t vdivq_f32(float32x4_t a, float32x4_t b) {
	float a0 = vgetq_lane_f32(a, 0), b0 = vgetq_lane_f32(b, 0);
	float a1 = vgetq_lane_f32(a, 1), b1 = vgetq_lane_f32(b, 1);
	float a2 = vgetq_lane_f32(a, 2), b2 = vgetq_lane_f32(b, 2);
	float a3 = vgetq_lane_f32(a, 3), b3 = vgetq_lane_f32(b, 3);
	float32x4_t c{ a0 / b0, a1 / b1, a2 / b2, a3 / b3 };
	return c;
}

/**
 * @brief Table vector lookup.
 */
ASTCENC_SIMD_INLINE int8x16_t vqtbl1q_s8(int8x16_t t, uint8x16_t idx) {
	int8x8x2_t tab;
	tab.val[0] = vget_low_s8(t);
	tab.val[1] = vget_high_s8(t);
	int8x16_t id = vreinterpretq_s8_u8(idx);
	return vcombine_s8(
			vtbl2_s8(tab, vget_low_s8(id)),
			vtbl2_s8(tab, vget_high_s8(id)));
}

ASTCENC_SIMD_INLINE uint32_t halfbits_to_floatbits(uint16_t h) {
	uint16_t h_exp, h_sig;
	uint32_t f_sgn, f_exp, f_sig;

	h_exp = (h & 0x7c00u);
	f_sgn = ((uint32_t)h & 0x8000u) << 16;
	switch (h_exp) {
		case 0x0000u: /* 0 or subnormal */
			h_sig = (h & 0x03ffu);
			/* Signed zero */
			if (h_sig == 0) {
				return f_sgn;
			}
			/* Subnormal */
			h_sig <<= 1;
			while ((h_sig & 0x0400u) == 0) {
				h_sig <<= 1;
				h_exp++;
			}
			f_exp = ((uint32_t)(127 - 15 - h_exp)) << 23;
			f_sig = ((uint32_t)(h_sig & 0x03ffu)) << 13;
			return f_sgn + f_exp + f_sig;
		case 0x7c00u: /* inf or NaN */
			/* All-ones exponent and a copy of the significand */
			return f_sgn + 0x7f800000u + (((uint32_t)(h & 0x03ffu)) << 13);
		default: /* normalized */
			/* Just need to adjust the exponent and shift */
			return f_sgn + (((uint32_t)(h & 0x7fffu) + 0x1c000u) << 13);
	}
}

ASTCENC_SIMD_INLINE float halfptr_to_float(const uint16_t *h) {
	union {
		uint32_t u32;
		float f32;
	} u;

	u.u32 = halfbits_to_floatbits(*h);
	return u.f32;
}

ASTCENC_SIMD_INLINE float16_t make_half_float(uint32_t f) {
	union {
		float fv;
		uint32_t ui;
	} ci;
	union {
		float16_t fv;
		uint16_t ui;
	} ci2;
	ci.ui = f;

	uint32_t x = ci.ui;
	uint32_t sign = (unsigned short)(x >> 31);
	uint32_t mantissa;
	uint32_t exp;
	uint16_t hf;

	// get mantissa
	mantissa = x & ((1 << 23) - 1);
	// get exponent bits
	exp = x & (0xFF << 23);
	if (exp >= 0x47800000) {
		// check if the original single precision float number is a NaN
		if (mantissa && (exp == (0xFF << 23))) {
			// we have a single precision NaN
			mantissa = (1 << 23) - 1;
		} else {
			// 16-bit half-float representation stores number as Inf
			mantissa = 0;
		}
		hf = (((uint16_t)sign) << 15) | (uint16_t)((0x1F << 10)) |
			 (uint16_t)(mantissa >> 13);
	}
	// check if exponent is <= -15
	else if (exp <= 0x38000000) {
		/*// store a denorm half-float value or zero
	exp = (0x38000000 - exp) >> 23;
	mantissa >>= (14 + exp);

	hf = (((uint16_t)sign) << 15) | (uint16_t)(mantissa);
	*/
		hf = 0; //denormals do not work for 3D, convert to zero
	} else {
		hf = (((uint16_t)sign) << 15) |
			 (uint16_t)((exp - 0x38000000) >> 13) |
			 (uint16_t)(mantissa >> 13);
	}
	ci2.ui = hf;
	return ci2.fv;
}
/**
 * @brief Horizontal integer addition.
 */
ASTCENC_SIMD_INLINE uint32_t vaddvq_u32(uint32x4_t a) {
	uint32_t a0 = vgetq_lane_u32(a, 0);
	uint32_t a1 = vgetq_lane_u32(a, 1);
	uint32_t a2 = vgetq_lane_u32(a, 2);
	uint32_t a3 = vgetq_lane_u32(a, 3);
	return a0 + a1 + a2 + a3;
}
#if !(__ARM_FP & 2)
ASTCENC_SIMD_INLINE float32x4_t vcvt_f32_f16(uint16x4_t a) {
	uint16_t a0 = vget_lane_u16(a, 0);
	uint16_t a1 = vget_lane_u16(a, 1);
	uint16_t a2 = vget_lane_u16(a, 2);
	uint16_t a3 = vget_lane_u16(a, 3);
	float32x4_t c{ halfptr_to_float((const uint16_t *)&a0), halfptr_to_float((const uint16_t *)&a1), halfptr_to_float((const uint16_t *)&a2), halfptr_to_float((const uint16_t *)&a3) };
	return c;
}
ASTCENC_SIMD_INLINE float16x4_t vcvt_f16_f32(uint32x4_t a) {
	uint32_t a0 = vgetq_lane_u32(a, 0);
	uint32_t a1 = vgetq_lane_u32(a, 1);
	uint32_t a2 = vgetq_lane_u32(a, 2);
	uint32_t a3 = vgetq_lane_u32(a, 3);
	float16x4_t c{ make_half_float(a0), make_half_float(a1), make_half_float(a2), make_half_float(a3) };
	return c;
}
#endif

#endif
