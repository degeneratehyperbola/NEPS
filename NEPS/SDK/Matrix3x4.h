#pragma once

struct Vector;

class Matrix3x4 {
    float mat[3][4];
public:
    constexpr auto operator[](int i) const noexcept { return mat[i]; }
    constexpr auto origin() const noexcept;
	constexpr void setOrigin(Vector in) noexcept;
};

#include "Vector.h"

constexpr auto Matrix3x4::origin() const noexcept
{
    return Vector{ mat[0][3], mat[1][3], mat[2][3] };
}

constexpr void Matrix3x4::setOrigin(Vector in) noexcept
{
	mat[0][3] = in.x;
	mat[1][3] = in.y;
	mat[2][3] = in.z;
}
