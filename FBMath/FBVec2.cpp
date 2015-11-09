#include "stdafx.h"
#include "FBVec2.h"
#include "FBMath.h"

namespace fastbird
{
	const Vec2 Vec2::UNIT_X(1.f, 0);
	const Vec2 Vec2::UNIT_Y(0, 1.f);
	const Vec2 Vec2::ZERO(0, 0);
	const Vec2 Vec2::ONE(1, 1);

	//-------------------------------------------------------------------
	Vec2::Vec2(){
	}

	Vec2::Vec2(Real _x, Real _y)
		:x(_x), y(_y)
	{
	}

	Vec2::Vec2(const Vec2I& v)
		:x((Real)v.x), y((Real)v.y)
	{
	}

	//-------------------------------------------------------------------
	Vec2 Vec2::operator+ (Real s) const
	{
		return Vec2(x + s, y + s);
	}

	Vec2& Vec2::operator+= (Real s)
	{
		x += s;
		y += s;
		return *this;
	}

	Vec2 Vec2::operator+ (const Vec2& v) const
	{
		return Vec2(x + v.x, y + v.y);
	}

	Vec2& Vec2::operator+= (const Vec2& s)
	{
		x += s.x;
		y += s.y;
		return *this;
	}

	Vec2 Vec2::operator- (Real s) const
	{
		return Vec2(x - s, y - s);
	}

	Vec2 Vec2::operator- () const
	{
		return Vec2(-x, -y);
	}

	Vec2 Vec2::operator- (const Vec2& v) const
	{
		return Vec2(x - v.x, y - v.y);
	}

	Vec2& Vec2::operator-= (Real s)
	{
		x -= s;
		y -= s;
		return *this;
	}

	Vec2 Vec2::operator* (Real s) const
	{
		return Vec2(x*s, y*s);
	}

	Vec2 Vec2::operator* (const Vec2& v) const
	{
		return Vec2(x*v.x, y*v.y);
	}

	Vec2& Vec2::operator*= (Real s)
	{
		x *= s;
		y *= s;
		return *this;
	}

	Vec2& Vec2::operator*= (const Vec2& v)
	{
		x *= v.x;
		y *= v.y;
		return *this;
	}

	Vec2 Vec2::operator/ (Real s) const
	{
		return Vec2(x / s, y / s);
	}	

	Vec2 Vec2::operator/ (const Vec2& v) const
	{
		return Vec2(x / v.x, y / v.y);
	}

	Vec2& Vec2::operator/= (const Vec2& other)
	{
		x /= other.x;
		y /= other.y;
		return *this;
	}

	Vec2& Vec2::operator/= (Real s)
	{
		x /= s;
		y /= s;
		return *this;
	}	

	bool Vec2::operator== (const Vec2& other) const
	{
		return x == other.x && y == other.y;
	}

	bool Vec2::operator!= (const Vec2& other) const
	{
		return x != other.x || y != other.y;
	}

	Real Vec2::operator[] (const size_t i) const
	{
		assert(i < 2);

		return *(&x + i);
	}

	Real& Vec2::operator[] (const size_t i)
	{
		assert(i < 2);

		return *(&x + i);
	}

	bool Vec2::operator<(const Vec2& other) const
	{
		if (x < other.x)
			return true;
		else if (IsEqual(x, other.x, EPSILON))
			return y<other.y;

		return false;
	}

	//--------------------------------------------------------------------------
	Real Vec2::Normalize()
	{
		Real length = std::sqrt(x*x + y*y);
		if (length > 0.0f)
		{
			Real invLength = 1.f / length;
			x *= invLength;
			y *= invLength;
		}

		return length;
	}

	Vec2 Vec2::NormalizeCopy() const
	{
		Vec2 result = *this;
		result.Normalize();
		return result;
	}

	void Vec2::SafeNormalize()
	{
		Vec2 absVec(Abs(*this));

		int maxIndex = absVec.MaxAxis();
		if (absVec[maxIndex]>0)
		{
			*this /= absVec[maxIndex];
			*this /= Length();
			return;
		}
		*this = Vec2(1, 0);
	}

	Real Vec2::Length() const
	{
		return std::sqrt(x*x + y*y);
	}

	Real Vec2::LengthSQ() const
	{
		return x*x + y*y;
	}

	Real Vec2::DistanceTo(const Vec2& other) const
	{
		return (*this - other).Length();
	}

	Real Vec2::DistanceToSQ(const Vec2& other) const
	{
		return (*this - other).LengthSQ();
	}

	Real Vec2::Cross(const Vec2& right){
		return x * right.y - y * right.x;
	}	

	int Vec2::MaxAxis() const
	{
		return x < y ? 1 : 0;
	}

	//--------------------------------------------------------------------------
	Vec2 operator / (const Vec2I& left, const Vec2& right){
		return Vec2(left.x / right.x, left.y / right.y);
	}

	Vec2 operator * (const Vec2I& left, const Vec2& right){
		return Vec2(left.x * right.x, left.y * right.y);
	}

	Vec2 operator * (const Vec2& left, const Vec2I& right){
		return Vec2(left.x * right.x, left.y * right.y);
	}

	Vec2 operator*(const Vec2I& left, Real right){
		return Vec2(left.x * right, left.y * right);
	}
}