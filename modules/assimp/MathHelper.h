//***************************************************************************************
// MathHelper.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Helper math class.
//***************************************************************************************

#pragma once
#include "core/math/vector3.h"
#include <cstdint>

class MathHelper {
public:
	// Returns random float in [0, 1).
	static float RandF() {
		return (float)(rand()) / (float)RAND_MAX;
	}

	// Returns random float in [a, b).
	static float RandF(float a, float b) {
		return a + RandF() * (b - a);
	}

	static int Rand(int a, int b) {
		return a + rand() % ((b - a) + 1);
	}

	template <typename T>
	static T Min(const T &a, const T &b) {
		return a < b ? a : b;
	}

	template <typename T>
	static T Max(const T &a, const T &b) {
		return a > b ? a : b;
	}

	template <typename T>
	static T Lerp(const T &a, const T &b, float t) {
		return a + (b - a) * t;
	}

	template <typename T>
	static T Clamp(const T &x, const T &low, const T &high) {
		return x < low ? low : (x > high ? high : x);
	}

	// Returns the polar angle of the point (x,y) in [0, 2*PI).
	static float AngleFromXY(float x, float y);

	static float getDistance(const Vector3 &v1, const Vector3 &v2);

	static const float Infinity;
	static const float Pi;
};
