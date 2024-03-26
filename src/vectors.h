#pragma once

struct float2
{
	float x;
	float y;
};

inline float2 operator+(const float2& a, const float2& b)
{
	return { a.x + b.x, a.y + b.y };
}
