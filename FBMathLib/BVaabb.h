#pragma once
#include "BoundingVolume.h"
#include "AABB.h"
namespace fastbird
{
	class BVaabb: public BoundingVolume
	{
	public:
		BVaabb();
		BVaabb& operator=(const BVaabb& other);
		
		virtual int GetBVType() const {return BV_AABB; }
		virtual void SetCenter (const Vec3& center);
		virtual void SetRadius (Real fRadius);
		virtual const Vec3& GetCenter () const;
		virtual Real GetRadius () const;

		virtual void ComputeFromData(const Vec3* pVertices, size_t numVertices);
		virtual void StartComputeFromData();
		virtual void AddComputeData(const Vec3* pVertices, size_t numVertices);
		virtual void AddComputeData(const Vec3& vert);
		virtual void EndComputeFromData();
		virtual void TransformBy(const Transformation& transform,
			BoundingVolumePtr result);
		virtual int WhichSide(const Plane3& plane) const;
		virtual bool TestIntersection(const Ray3& ray) const;
		virtual bool TestIntersection(BoundingVolumePtr pBV) const;

		virtual void Merge(const BoundingVolumePtr pBV);
		virtual void Merge(const Vec3& worldPos);

		virtual fastbird::Vec3 GetSurfaceFrom(const Vec3& source, Vec3& normal);
		virtual void Invalidate(){ mAABB.Invalidate(); }

		void SetAABB(const AABB& aabb);
		const AABB& GetAABB() const { return mAABB; }

		void Expand(Real e);
		virtual bool Contain(const Vec3& pos) const;
		virtual Vec3 GetRandomPosInVolume(const Vec3* nearLocal = 0) const;
	

	private:
		AABB mAABB;
		Vec3 mCenter;
		Real mRadius;
	};
}