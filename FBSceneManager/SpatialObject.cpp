#include "stdafx.h"
#include "SpatialObject.h"
#include "FBMathLib/BoundingVolume.h"
#include "Animation.h"
#include "AnimationData.h"
using namespace fastbird;

class SpatialObject::Impl{
public:
	Transformation mLocation;
	TransformationPtr mAnimatedLocation;
	BoundingVolumePtr mBoundingVolume;
	BoundingVolumePtr mBoundingVolumeWorld;
	Real mDistToCam;
	AnimationPtr mAnim;
	bool mTransformChanged;


	//---------------------------------------------------------------------------
	Impl()
		: mBoundingVolume(BoundingVolume::Create())
		, mBoundingVolumeWorld(BoundingVolume::Create())
	{
	}

	Impl(const Impl& other)
		:Impl()
	{
		mLocation = other.mLocation;
		if (other.mAnimatedLocation){
			mAnimatedLocation = Transformation::Create();
			*mAnimatedLocation = *other.mAnimatedLocation;
		}
		*mBoundingVolume = *other.mBoundingVolume;
		*mBoundingVolumeWorld = *other.mBoundingVolumeWorld;
		mDistToCam = other.mDistToCam;
		if (other.mAnim){
			mAnim = other.mAnim->Clone();
		}
		mTransformChanged = other.mTransformChanged;
	}

	void SetRadius(Real r){
		mBoundingVolume->SetRadius(r);
		mBoundingVolumeWorld->SetRadius(r);
	}

	Real GetRadius() const{
		return mBoundingVolumeWorld->GetRadius();
	}

	void SetDistToCam(Real dist){
		mDistToCam = dist;
	}

	Real GetDistToCam() const{
		return mDistToCam;
	}

	const Vec3& GetPosition() const{
		return mLocation.GetTranslation();
	}

	const Vec3& GetScale() const{
		return mLocation.GetScale();
	}

	void SetPosition(const Vec3& pos){
		mLocation.SetTranslation(pos);
	}

	BoundingVolumePtr GetBoundingVolume(){
		return mBoundingVolume;
	}

	BoundingVolumePtr GetBoundingVolumeWorld(){
		return mBoundingVolumeWorld;
	}

	const Transformation& GetLocation() const{
		return mLocation;
	}

	const Transformation& GetAnimatedLocation() const{
		return mAnim ? *mAnimatedLocation : mLocation;
	}

	void SetLocation(const Transformation& t){
		mLocation = t;
	}

	bool GetTransformChanged() const{
		return mTransformChanged;
	}

	void ClearTransformChanged(){
		mTransformChanged = false;
	}

	void SetAnimation(AnimationPtr anim){
		mAnim = anim;
		if (!mAnimatedLocation){
			mAnimatedLocation = Transformation::Create();
		}
		mAnimatedLocation->MakeIdentity();
	}

	void UpdateAnimation(TIME_PRECISION dt){
		if (mAnim){
			mAnim->Update(dt);
			if (mAnim->Changed())
				*mAnimatedLocation = mLocation * mAnim->GetResult();
		}
	}

	bool IsPlayingAction() const{
		return mAnim && mAnim->IsPlaying();
	}
};

//---------------------------------------------------------------------------
SpatialObject::SpatialObject()
	: mImpl(new Impl)
{
}

SpatialObject::SpatialObject(const SpatialObject& other)
	: SceneObject(other)
	, mImpl(new Impl(*other.mImpl)){

}

SpatialObject::~SpatialObject(){

}

void SpatialObject::SetRadius(Real r){
	mImpl->SetRadius(r);
}

Real SpatialObject::GetRadius() const{
	return mImpl->GetRadius();
}

void SpatialObject::SetDistToCam(Real dist){
	mImpl->SetDistToCam(dist);
}

Real SpatialObject::GetDistToCam() const{
	return mImpl->GetDistToCam();
}

const Vec3& SpatialObject::GetPosition() const{
	return mImpl->GetPosition();
}

const Vec3& SpatialObject::GetScale() const{
	return mImpl->GetScale();
}

void SpatialObject::SetPosition(const Vec3& pos){
	mImpl->SetPosition(pos);
}

BoundingVolumePtr SpatialObject::GetBoundingVolume(){
	return mImpl->GetBoundingVolume();
}

BoundingVolumePtr SpatialObject::GetBoundingVolumeWorld(){
	return mImpl->GetBoundingVolumeWorld();
}


const Transformation& SpatialObject::GetLocation() const{
	return mImpl->GetLocation();
}

const Transformation& SpatialObject::GetAnimatedLocation() const{
	return mImpl->GetAnimatedLocation();
}

void SpatialObject::SetLocation(const Transformation& t){
	mImpl->SetLocation(t);
}

bool SpatialObject::GetTransformChanged() const{
	mImpl->GetTransformChanged();
}

void SpatialObject::ClearTransformChanged(){
	mImpl->ClearTransformChanged();
}

void SpatialObject::SetAnimation(AnimationPtr anim){
	mImpl->SetAnimation(anim);
}

void SpatialObject::UpdateAnimation(TIME_PRECISION dt){
	mImpl->UpdateAnimation(dt);
}

bool SpatialObject::IsPlayingAction() const{
	return mImpl->IsPlayingAction();
}