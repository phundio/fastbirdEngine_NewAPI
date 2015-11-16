#pragma once

#include "Vec3.h"
#include "Vec3I.h"
#include "Plane3.h"

namespace fastbird
{
	DECLARE_SMART_PTR(BoundingVolume);
	class AABB;
	class Ray3
	{
		Vec3 mOrigin;
		Vec3 mDir;
		Vec3 mDirInv;
		Vec3I mSigns;

	public:
		Ray3();
		Ray3(const Vec3& origin, const Vec3& dir);

		// IntersectionResult
		typedef std::pair<bool, Real> IResult;
		IResult Intersects(const BoundingVolume* pBoundingVolume) const;
		IResult Intersects(const AABB& aabb, Vec3& normal) const;
		IResult Intersects(const Plane3& p) const;

		const Vec3& GetDir() const { return mDir; }
		const Vec3& GetOrigin() const { return mOrigin; }
		void SetOrigin(const Vec3& v) { mOrigin = v; }
		void SetDir(const Vec3& dir);
		Vec3 GetPoint(Real dist) const { return mOrigin + mDir * dist; }
		void AddOffset(const Vec3& v) { mOrigin  += v;}		
	};
}