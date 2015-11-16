#include "stdafx.h"
#include "Ray3.h"
#include "Math.h"
#include "BoundingVolume.h"

namespace fastbird
{

Ray3::Ray3()
{
}

Ray3::Ray3(const Vec3& origin, const Vec3& dir)
	: mOrigin(origin)
{
	SetDir(dir);
}

Ray3::IResult Ray3::Intersects(const BoundingVolume* pBoundingVolume) const
{
	assert(pBoundingVolume);
    // to relative space
	Vec3 origin = mOrigin - pBoundingVolume->GetCenter();
	Real radius = pBoundingVolume->GetRadius();

    // Check origin inside first
	if (origin.LengthSQ() <= radius*radius)
    {
		return Ray3::IResult(true, 0.f);
    }

    // t = (-b +/- sqrt(b*b + 4ac)) / 2a
	Real a = mDir.Dot(mDir);
	Real b = 2 * origin.Dot(mDir);
    Real c = origin.Dot(origin) - radius*radius;

    // determinant
    Real d = (b*b) - (4 * a * c);
    if (d < 0)
    {
        // No intersection
        return IResult(false, 0.f);
    }
    else
    {
        Real t = ( -b - sqrt(d) ) / (2 * a);

        if (t < 0)
            t = ( -b + sqrt(d) ) / (2 * a);
        return IResult(true, t);
    }
}

Ray3::IResult Ray3::Intersects(const Plane3& p) const
{
	Real denom = p.mNormal.Dot(mDir);
	if (abs(denom) < std::numeric_limits<Real>::epsilon())
	{
		return IResult(false, 0.f);
	}
	else
	{
		Real nom = p.mNormal.Dot(mOrigin) + p.mConstant;
		Real t = -(nom/denom);
		return IResult(t>=0, t);
	}
}

Ray3::IResult Ray3::Intersects(const AABB& aabb, Vec3& normal) const
{
	Real min = 1.0f;
	Real pseudo_min = 0.0f;
	Real pseudo_max = 1000.0f;
	bool collide = RayAABB(mOrigin, mDirInv, mSigns, aabb, min, normal, pseudo_min, pseudo_max);
	return IResult(collide, min);
}


void Ray3::SetDir(const Vec3& dir)
{ 
	mDir = dir; 
	mDirInv.x = mDir.x == 0.0f ? LARGE_Real : 1.0f / mDir.x;
	mDirInv.y = mDir.y == 0.0f ? LARGE_Real : 1.0f / mDir.y;
	mDirInv.z = mDir.z == 0.0f ? LARGE_Real : 1.0f / mDir.z;
	mSigns = Vec3I(mDir.x < 0.0 ? 1 : 0, 
		mDir.y < 0.0 ? 1 : 0, 
		mDir.z < 0.0 ? 1 : 0);
}

}