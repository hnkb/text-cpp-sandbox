
#pragma once

#include <cmath>
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

static constexpr inline float2 operator+(float a, float2 b) { return b + a; }
static constexpr inline float2 operator*(float a, float2 b) { return b * a; }
static constexpr inline float2 operator-(float a, float2 b) { return float2(a - b.x, a - b.y); }
static constexpr inline float2 operator/(float a, float2 b) { return float2(a / b.x, a / b.y); }

static constexpr inline float2& operator+=(float2& a, float2 b) { a.x += b.x; a.y += b.y; return a; }
static constexpr inline float2& operator-=(float2& a, float2 b) { a.x -= b.x; a.y -= b.y; return a; }
static constexpr inline float2& operator*=(float2& a, float2 b) { a.x *= b.x; a.y *= b.y; return a; }
static constexpr inline float2& operator/=(float2& a, float2 b) { a.x /= b.x; a.y /= b.y; return a; }

static constexpr inline float2& operator+=(float2& a, float b) { a.x += b; a.y += b; return a; }
static constexpr inline float2& operator-=(float2& a, float b) { a.x -= b; a.y -= b; return a; }
static constexpr inline float2& operator*=(float2& a, float b) { a.x *= b; a.y *= b; return a; }
static constexpr inline float2& operator/=(float2& a, float b) { a.x /= b; a.y /= b; return a; }

static constexpr inline bool operator==(const float2& a, const float2& b) { return a.x == b.x && a.y == b.y; }
static constexpr inline bool operator!=(float2 a, float2 b) { return !(a == b); }

static constexpr inline float2 operator-(float2 a) { return { -a.x, -a.y }; }
// clang-format on


// clang-format off
static constexpr inline int2 operator+(const int2& a, const int2& b) { return { a.x + b.x, a.y + b.y }; }
static constexpr inline int2 operator-(int2 a, int2 b) { return { a.x - b.x, a.y - b.y }; }
// clang-format on


// clang-format off
static constexpr inline float2 abs(float2 a) { return float2(abs(a.x), abs(a.y)); }
static constexpr inline float2 floor(float2 a) { return float2(floor(a.x), floor(a.y)); }
static constexpr inline float2 ceil(float2 a) { return float2(ceil(a.x), ceil(a.y)); }
static constexpr inline float2 round(float2 a) { return float2(round(a.x), round(a.y)); }
static constexpr inline float2 sqrt(float2 a) { return float2(sqrt(a.x), sqrt(a.y)); }
static constexpr inline float2 fmin(float2 a, float2 b) { return float2(fmin(a.x, b.x), fmin(a.y, b.y)); }
static constexpr inline float2 fmin(float2 a, float b) { return float2(fmin(a.x, b), fmin(a.y, b)); }
static constexpr inline float2 fmax(float2 a, float2 b) { return float2(fmax(a.x, b.x), fmax(a.y, b.y)); }
static constexpr inline float2 fmax(float2 a, float b) { return float2(fmax(a.x, b), fmax(a.y, b)); }
static constexpr inline float2 fmin(float a, float2 b) { return fmin(b, a); }
static constexpr inline float2 fmax(float a, float2 b) { return fmax(b, a); }

static constexpr inline float2 sin(float2 a) { return float2(sin(a.x), sin(a.y)); }
static constexpr inline float2 cos(float2 a) { return float2(cos(a.x), cos(a.y)); }

static constexpr inline float dot(float2 a, float2 b) { return a.x * b.x + a.y * b.y; }
static constexpr inline float cross(float2 a, float2 b) { return a.x * b.y - a.y * b.x; }
// clang-format on


template <class Type>
static inline void sincos(Type a, Type& out_sin, Type& out_cos)
{
	out_sin = sin(a), out_cos = cos(a);
}

template <class A, class B, class C>
static inline A clamp(A a, B smallestValue, C largestValue)
{
	return fmax(smallestValue, fmin(a, largestValue));
}

template <class Type>
static inline Type frac(Type a)
{
	return a - floor(a);
}

template <class Type>
static inline Type lerp(Type a, Type b, decltype(Type::x) t)
{
	return (1 - t) * a + t * b;
}

template <class Type>
static inline Type lerp(Type a, Type b, Type t)
{
	return (1 - t) * a + t * b;
}

template <class Type>
static inline auto length(Type v)
{
	return sqrt(dot(v, v));
}

template <class Type>
static inline Type normalize(Type v)
{
	return v / length(v);
}

template <class Type>
static inline auto innerAngle(Type a, Type b)
{
	return acos(clamp(
		dot(a, b) / sqrt(dot(a, a) * dot(b, b)),
		(decltype(Type::x))-1,
		(decltype(Type::x))1));
}


#if 1
#	include <ostream>
template <class CharType>
inline auto& operator<<(std::basic_ostream<CharType, std::char_traits<CharType>>& s, float2 v)
{
	return s << "(" << v.x << ", " << v.y << ")";
}
#endif


static inline float2
	bezier(float t, const float2& P0, const float2& P1, const float2& P2, const float2& P3)
{
	const float u = 1 - t;
	const float tt = t * t;
	const float uu = u * u;
	return uu * u * P0 + 3 * uu * t * P1 + 3 * u * tt * P2 + t * tt * P3;
}
