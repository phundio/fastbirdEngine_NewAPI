#include "stdafx.h"
#include "Animation.h"
#include "AnimationData.h"
#include "FBMathLib/Math.h"
#include "FBStringLib/StringLib.h"
#include "FBTimer/Timer.h"
using namespace fastbird;

class Animation::Impl{
public:
	AnimationDataPtr mAnimationData;
	AnimationData::Action* mCurPlayingAction;
	AnimationData::Action* mNextAction;
	float mPrevPlayingTime;
	float mPlayingTime;
	bool mCycled; // true when looped.
	bool mReverse;
	bool mNextReverse;

	Transformation mResult;
	unsigned mLastUpdatedFrame;
	bool mChanged;
	// Using Default Copy Constructor!

	//---------------------------------------------------------------------------
	Impl(){
	}

	// default copy ctor is fine.
	/*Impl(const Impl& other){
	}*/


	void PlayAction(const std::string& name, bool immediate, bool reverse){
		assert(mAnimationData);
		auto action = mAnimationData->GetAction(name.c_str());
		if (!action){
			Logger::Log(FB_ERROR_LOG_ARG, FormatString("Cannot find an action (%s) in animation(%s)", name.c_str(), mAnimationData->GetName()).c_str());
			return;
		}
		if (immediate || !mCurPlayingAction ||
			(mReverse && mPlayingTime == 0.f) ||
			(!mReverse && mPlayingTime == mCurPlayingAction->mLength) ||
			mCurPlayingAction->mLoop
			)
		{
			mCurPlayingAction = action;
			mReverse = reverse;
			if (reverse)
			{
				mPlayingTime = mCurPlayingAction->mLength;
				mPrevPlayingTime = 0.0f;
			}
			else
			{
				mPlayingTime = 0.f;
				mPrevPlayingTime = mCurPlayingAction->mLength;
			}
			mNextAction = 0;
		}
		else
		{
			mNextAction = action;
			mNextReverse = reverse;
		}
	}

	bool IsActionDone(const char* action) const{
		if (mCurPlayingAction && strcmp(mCurPlayingAction->mName.c_str(), action) == 0)
		{
			if (mCurPlayingAction->mLoop)
				return true;
			if (mReverse)
			{
				return mPlayingTime == 0.0f;
			}
			else
			{
				return mPlayingTime == mCurPlayingAction->mLength;
			}
		}
		if (strcmp(mNextAction->mName.c_str(), action) == 0)
			return false;

		return true;
	}

	bool IsPlaying() const{
		if (mCurPlayingAction)
		{
			if (mCurPlayingAction->mLoop)
				return true;
			if (mReverse)
			{
				return mPlayingTime > 0.0f;
			}
			else
			{
				return mPlayingTime <= mCurPlayingAction->mLength;
			}
		}
		return false;
	}

	void Update(float dt){
		if (mCurPlayingAction)
		{
			if (mLastUpdatedFrame == gpTimer->GetFrame())
				return;

			mChanged = false;

			if (mPrevPlayingTime == mPlayingTime)
				return;

			float curTime = mPlayingTime;
			if (mReverse)
				mPlayingTime -= dt;
			else
				mPlayingTime += dt;

			mLastUpdatedFrame = gpTimer->GetFrame();
			// evaluate
			float normTime = curTime / mCurPlayingAction->mLength;
			bool cycled = mCycled;
			mCycled = false;
			if (mAnimationData->HasPosAnimation())
			{
				const Vec3 *p1 = 0, *p2 = 0;
				float interpol = 0;
				mAnimationData->PickPos(curTime, cycled, &p1, &p2, interpol);
				if (p1 && p2)
				{
					Vec3 pos = Lerp<Vec3>(*p1, *p2, interpol);
					mResult.SetTranslation(pos);
				}
			}

			if (mAnimationData->HasRotAnimation())
			{
				const Quat *r1 = 0, *r2 = 0;
				float interpol = 0;
				mAnimationData->PickRot(curTime, cycled, &r1, &r2, interpol);
				if (r1 && r2)
				{
					Quat rot = Slerp(*r1, *r2, interpol);
					mResult.SetRotation(rot);
				}
			}

			mPrevPlayingTime = curTime;
			mChanged = true;

			if ((!mReverse && mPlayingTime > mCurPlayingAction->mLength) ||
				(mReverse && mPlayingTime < 0))
			{
				if (mNextAction)
				{
					mCurPlayingAction = mNextAction;
					mReverse = mNextReverse;
					mNextAction = 0;
					if (mReverse)
					{
						mPlayingTime = mCurPlayingAction->mLength;
						mPrevPlayingTime = 0;
					}
					else
					{
						mPlayingTime = 0.f;
						mPrevPlayingTime = mCurPlayingAction->mLength;
					}
				}
				else
				{
					if (mReverse)
					{
						if (mCurPlayingAction->mLoop)
						{
							// mPlayingTime is negative
							mPlayingTime = mCurPlayingAction->mLength + mPlayingTime;
							mCycled = true;
						}
						else
						{
							mPlayingTime = 0.0f;
						}
					}
					else
					{
						if (mCurPlayingAction->mLoop)
						{
							mPlayingTime = mPlayingTime - mCurPlayingAction->mLength;
							mCycled = true;
						}
						else
						{
							mPlayingTime = mCurPlayingAction->mLength;
						}
					}

				}
			}
		}
	}

	const Transformation& GetResult() const { 
		return mResult; 
	}

	bool Changed() const { 
		return mChanged; 
	}

	void SetAnimationData(AnimationDataPtr data) { 
		mAnimationData = data; 
	}
};

//---------------------------------------------------------------------------
IMPLEMENT_STATIC_CREATE(Animation);
AnimationPtr Animation::Create(const Animation& other){
	return AnimationPtr(new Animation(other), [](Animation* obj){ delete obj; });
}

Animation::Animation()
	: mImpl(new Impl){
}

Animation::Animation(const Animation& other)
	: mImpl(new Impl(*other.mImpl)){
}

Animation::~Animation(){
}

AnimationPtr Animation::Clone() const{
	auto cloned = Animation::Create(*this);
	return cloned;
}

void Animation::PlayAction(const std::string& name, bool immediate, bool reverse){
	mImpl->PlayAction(name, immediate, reverse);
}

bool Animation::IsActionDone(const char* action) const{
	return mImpl->IsActionDone(action);
}

bool Animation::IsPlaying() const{
	return mImpl->IsPlaying();
}

void Animation::Update(float dt){
	mImpl->Update(dt);
}

const Transformation& Animation::GetResult() const{
	return mImpl->GetResult();
}

bool Animation::Changed() const{
	return mImpl->Changed();
}

void Animation::SetAnimationData(AnimationDataPtr data){
	mImpl->SetAnimationData(data);
}