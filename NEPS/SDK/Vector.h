#pragma once

#include <cmath>

#include "../lib/Helpers.hpp"

class Matrix3x4;

struct Vector
{
	constexpr auto notNull() const noexcept
	{
		return x || y || z;
	}

	constexpr auto operator==(const Vector &v) const noexcept
	{
		return x == v.x && y == v.y && z == v.z;
	}

	constexpr auto operator>=(const Vector &v) const noexcept
	{
		return x >= v.x && y >= v.y && z >= v.z;
	}

	constexpr auto operator<=(const Vector &v) const noexcept
	{
		return x <= v.x && y <= v.y && z <= v.z;
	}

	constexpr auto operator>(const Vector &v) const noexcept
	{
		return x > v.x && y > v.y && z > v.z;
	}

	constexpr auto operator<(const Vector &v) const noexcept
	{
		return x < v.x &&y < v.y &&z < v.z;
	}

	constexpr auto operator!=(const Vector &v) const noexcept
	{
		return !(*this == v);
	}

	constexpr Vector &operator=(const float array[3]) noexcept
	{
		x = array[0];
		y = array[1];
		z = array[2];
		return *this;
	}

	constexpr Vector &operator+=(const Vector &v) noexcept
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	constexpr Vector &operator+=(float f) noexcept
	{
		x += f;
		y += f;
		z += f;
		return *this;
	}

	constexpr Vector &operator-=(const Vector &v) noexcept
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	constexpr Vector &operator-=(float f) noexcept
	{
		x -= f;
		y -= f;
		z -= f;
		return *this;
	}

	constexpr auto operator-(const Vector &v) const noexcept
	{
		return Vector{x - v.x, y - v.y, z - v.z};
	}

	constexpr auto operator+(const Vector &v) const noexcept
	{
		return Vector{x + v.x, y + v.y, z + v.z};
	}

	constexpr auto operator*(const Vector &v) const noexcept
	{
		return Vector{x * v.x, y * v.y, z * v.z};
	}

	constexpr Vector &operator/=(float div) noexcept
	{
		x /= div;
		y /= div;
		z /= div;
		return *this;
	}

	constexpr Vector &operator*=(float mul) noexcept
	{
		x *= mul;
		y *= mul;
		z *= mul;
		return *this;
	}

	constexpr auto operator*(float mul) const noexcept
	{
		return Vector{x * mul, y * mul, z * mul};
	}

	constexpr auto operator/(float div) const noexcept
	{
		return Vector{x / div, y / div, z / div};
	}

	constexpr auto operator-(float sub) const noexcept
	{
		return Vector{x - sub, y - sub, z - sub};
	}

	constexpr auto operator+(float add) const noexcept
	{
		return Vector{x + add, y + add, z + add};
	}

	constexpr auto operator-() const noexcept
	{
		return Vector{-x, -y, -z};
	}

	constexpr Vector &normalize() noexcept
	{
		x = Helpers::normalizeDeg(x);
		y = Helpers::normalizeDeg(y);
		z = 0.0f;
		return *this;
	}

	auto length() const noexcept
	{
		return std::sqrt(x * x + y * y + z * z);
	}

	auto length2D() const noexcept
	{
		return std::sqrt(x * x + y * y);
	}

	constexpr auto lengthSquared() const noexcept
	{
		return x * x + y * y + z * z;
	}

	constexpr auto lengthSquared2D() const noexcept
	{
		return x * x + y * y;
	}

	constexpr auto dotProduct(const Vector &v) const noexcept
	{
		return x * v.x + y * v.y + z * v.z;
	}

	constexpr auto dotProduct2D(const Vector &v) const noexcept
	{
		return x * v.x + y * v.y;
	}

	constexpr auto crossProduct(const Vector &v) const noexcept
	{
		return Vector{y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};
	}

	auto rotate(const Vector &axis, float angle) const noexcept
	{
		const auto cos = std::cos(Helpers::degreesToRadians(angle));
		const auto sin = std::sin(Helpers::degreesToRadians(angle));
		return *this * cos + axis.crossProduct(*this) * sin + axis * axis.dotProduct(*this) * (1.0f - cos);
	}

	constexpr auto transform(const Matrix3x4 &mat) const noexcept;

	auto distTo(const Vector &v) const noexcept
	{
		return (*this - v).length();
	}

	constexpr auto distToSquared(const Vector &v) const noexcept
	{
		return (*this - v).lengthSquared();
	}

	constexpr auto toAngle() const noexcept
	{
		return Vector{Helpers::radiansToDegrees(std::atan2f(-z, std::hypot(x, y))),
					  Helpers::radiansToDegrees(std::atan2f(y, x)),
					  0.0f};
	}

	constexpr auto toAngle2D() const noexcept
	{
		return Helpers::radiansToDegrees(std::atan2f(y, x));
	}

	auto snapTo4() const noexcept
	{
		const float l = length2D();
		bool xp = x >= 0.0f;
		bool yp = y >= 0.0f;
		bool xy = std::fabsf(x) >= std::fabsf(y);
		if (xp && xy)
			return Vector{l, 0.0f, 0.0f};
		if (!xp && xy)
			return Vector{-l, 0.0f, 0.0f};
		if (yp && !xy)
			return Vector{0.0f, l, 0.0f};
		if (!yp && !xy)
			return Vector{0.0f, -l, 0.0f};

		return Vector{};
	}

	static auto fromAngle(const Vector &angle) noexcept
	{
		return Vector{std::cos(Helpers::degreesToRadians(angle.x)) * std::cos(Helpers::degreesToRadians(angle.y)),
					  std::cos(Helpers::degreesToRadians(angle.x)) * std::sin(Helpers::degreesToRadians(angle.y)),
					  -std::sin(Helpers::degreesToRadians(angle.x))};
	}

	static auto fromAngle2D(float angle) noexcept
	{
		return Vector{std::cos(Helpers::degreesToRadians(angle)),
					  std::sin(Helpers::degreesToRadians(angle)),
					  0.0f};
	}

	constexpr static auto up() noexcept
	{
		return Vector{0.0f, 0.0f, 1.0f};
	}

	constexpr static auto down() noexcept
	{
		return Vector{0.0f, 0.0f, -1.0f};
	}

	constexpr static auto forward() noexcept
	{
		return Vector{1.0f, 0.0f, 0.0f};
	}

	constexpr static auto back() noexcept
	{
		return Vector{-1.0f, 0.0f, 0.0f};
	}

	constexpr static auto left() noexcept
	{
		return Vector{0.0f, 1.0f, 0.0f};
	}

	constexpr static auto right() noexcept
	{
		return Vector{0.0f, -1.0f, 0.0f};
	}

	float x, y, z;
};

#include "Matrix3x4.h"

constexpr auto Vector::transform(const Matrix3x4 &mat) const noexcept
{
	return Vector{dotProduct({ mat[0][0], mat[0][1], mat[0][2] }) + mat[0][3],
				  dotProduct({ mat[1][0], mat[1][1], mat[1][2] }) + mat[1][3],
				  dotProduct({ mat[2][0], mat[2][1], mat[2][2] }) + mat[2][3]};
}
