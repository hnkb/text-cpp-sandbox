
#pragma once

#include <limits>


struct byte4
{
	unsigned char x, y, z, w;
};

struct int2
{
	int x, y;
};

struct int3
{
	int x, y, z;
};

struct int4
{
	int x, y, z, w;
};


struct alignas(8) float2
{
	float x, y;

	float2() = default;
	constexpr float2(float x, float y) : x(x), y(y) {}

	explicit constexpr float2(float s) : x(s), y(s) {}
	explicit constexpr float2(int2 vec) : x((float)vec.x), y((float)vec.y) {}

	explicit constexpr operator int2() const { return { (int)x, (int)y }; }

	static constexpr float2 nan() { return float2(std::numeric_limits<float>::quiet_NaN()); }
};

struct float3
{
	float x, y, z;
};

struct float4
{
	float x, y, z, w;
};


struct float3x3
{
	float3 c0, c1, c2;
};

struct float4x4
{
	float4 c0, c1, c2, c3;
};


// clang-format off
static constexpr inline float2 operator+(float2 a, float2 b) { return { a.x + b.x, a.y + b.y }; }
static constexpr inline float2 operator-(float2 a, float2 b) { return { a.x - b.x, a.y - b.y }; }
static constexpr inline float2 operator*(float2 a, float2 b) { return { a.x * b.x, a.y * b.y }; }
static constexpr inline float2 operator/(float2 a, float2 b) { return { a.x / b.x, a.y / b.y }; }

static constexpr inline float2 operator+(float2 v, float s) { return { v.x + s, v.y + s }; }
static constexpr inline float2 operator-(float2 v, float s) { return { v.x - s, v.y - s }; }
static constexpr inline float2 operator*(float2 v, float s) { return { v.x * s, v.y * s }; }
static constexpr inline float2 operator/(float2 v, float s) { return { v.x / s, v.y / s }; }

static constexpr inline float2& operator+=(float2& a, float2 b) { a.x += b.x; a.y += b.y; return a; }
static constexpr inline float2& operator-=(float2& a, float2 b) { a.x -= b.x; a.y -= b.y; return a; }

static constexpr inline bool operator==(const float2& a, const float2& b) { return a.x == b.x && a.y == b.y; }


static constexpr inline int2 operator+(const int2& a, const int2& b) { return { a.x + b.x, a.y + b.y }; }
static constexpr inline int2 operator-(int2 a, int2 b) { return { a.x - b.x, a.y - b.y }; }
// clang-format on
