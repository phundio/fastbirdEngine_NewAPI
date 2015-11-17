#include "stdafx.h"
#include "AnimationData.h"
#include "FBCommonHeaders/VectorMap.h"
#include "FBCommonHeaders/Helpers.h"
#include "FBMathLib/Math.h"
#include "FBStringLib/StringLib.h"
#include "FBStringLib/StringConverter.h"
#include "TinyXmlLib/tinyxml2.h"
using namespace fastbird;
class AnimationData::Impl{
public:
	friend class Animation;
	std::string mName;
	fastbird::VectorMap<float, Vec3> mScale;
	fastbird::VectorMap<float, Quat> mRot;
	fastbird::VectorMap<float, Vec3> mEuler;
	fastbird::VectorMap<float, Vec3> mPos;
	fastbird::VectorMap<std::string, Action> mActions;

	//---------------------------------------------------------------------------
	void AddPosition(float time, float v, PosComp comp){
		auto itFind = mPos.Find(time);
		if (itFind == mPos.end())
		{
			auto prevPos = FindPos(time);
			Vec3 def = Vec3::ZERO;
			if (prevPos)
				def = *prevPos;
			mPos[time] = def;
		}
		switch (comp)
		{
		case PosComp::X:
			mPos[time].x = v;
			break;
		case PosComp::Y:
			mPos[time].y = v;
			break;
		case PosComp::Z:
			mPos[time].z = v;
			break;
		}
	}

	void AddScale(float time, float v, PosComp comp){
		auto itFind = mScale.Find(time);
		if (itFind == mScale.end())
		{
			auto prev = FindScale(time);
			Vec3 def = Vec3::ZERO;
			if (prev)
				def = *prev;
			mScale[time] = def;
		}

		switch (comp)
		{
		case PosComp::X:
			mScale[time].x = v;
			break;
		case PosComp::Y:
			mScale[time].y = v;
			break;
		case PosComp::Z:
			mScale[time].z = v;
			break;
		}
	}

	void AddRotEuler(float time, float v, PosComp comp){
		auto itFind = mEuler.Find(time);
		if (itFind == mEuler.end())
		{
			auto prev = FindRotEuler(time);
			Vec3 def = Vec3::ZERO;
			if (prev)
				def = *prev;
			mEuler[time] = def;
		}

		switch (comp)
		{
		case PosComp::X:
			mEuler[time].x = v;
			break;
		case PosComp::Y:
			mEuler[time].y = v;
			break;
		case PosComp::Z:
		{
			mEuler[time].z = v;
			break;
		}
		}
	}

	bool HasPosAnimation() const{
		return !mPos.empty();
	}

	bool HasRotAnimation() const{
		return !mRot.empty();
	}

	bool HasScaleAnimation() const{
		return !mScale.empty();
	}

	void SetName(const char* name) { 
		if (name)
			mName = name; 
	}

	const char* GetName() const { 
		return mName.c_str(); 
	}

	void PickPos(float time, bool cycled, const Vec3** prev, const Vec3** next, float& interpol){
		int i = 0;
		*prev = &mPos.begin()->second;
		*next = *prev;
		float prevTime = mPos.begin()->first;
		for (const auto& it : mPos)
		{
			if (it.first <= time)
			{
				prevTime = it.first;
				*prev = &it.second;
			}
			if (it.first >= time)
			{
				*next = &it.second;
				float length = it.first - prevTime;
				if (length != 0.f)
					interpol = (time - prevTime) / length;
				return;
			}
			i++;
		}
	}

	void PickRot(float time, bool cycled, const Quat** prev, const Quat** next, float& interpol){
		int i = 0;
		*prev = &mRot.begin()->second;
		*next = *prev;
		float prevTime = mRot.begin()->first;
		for (const auto& it : mRot)
		{
			if (it.first <= time)
			{
				prevTime = it.first;
				*prev = &it.second;
			}
			if (it.first >= time)
			{
				*next = &it.second;
				float length = it.first - prevTime;
				if (length != 0.f)
					interpol = (time - prevTime) / length;
				return;
			}
			i++;
		}
	}

	void ToLocal(const Transformation& tolocal){
		for (auto& it : mPos)
		{
			it.second = tolocal.ApplyForward(it.second);
		}

		for (auto& it : mRot)
		{
			it.second = tolocal.GetRotation() * it.second;
		}

		for (auto& it : mScale)
		{
			it.second = tolocal.GetScale() * it.second;
		}
	}

	bool ParseAction(const char* filename){
		if (!ValidCStringLength(filename)){
			Logger::Log(FB_ERROR_LOG_ARG, "Invalid arg.");
			return false;
		}
		GenerateQuatFromEuler();

		tinyxml2::XMLDocument doc;
		int errId = doc.LoadFile(filename);
		if (errId)
		{
			Logger::Log(FB_ERROR_LOG_ARG, FormatString("AnimationData::ParseAction err : %s", doc.GetErrorStr1()).c_str());
			Logger::Log(FB_ERROR_LOG_ARG, doc.GetErrorStr2());
			return false;
		}
		auto actions = doc.FirstChildElement("Actions");
		if (!actions)
		{
			Logger::Log(FB_ERROR_LOG_ARG, FormatString("Cannot find 'Action' tag(%s)", filename).c_str());
			return false;
		}

		auto action = actions->FirstChildElement("Action");
		while (action)
		{
			const char* sz = action->Attribute("name");
			if (!sz)
			{
				Logger::Log(FB_ERROR_LOG_ARG, FormatString("Cannot find 'name' attribute(%s)", filename).c_str());
				return false;
			}
			auto& newAction = mActions[sz];
			newAction.mName = sz;
			sz = action->Attribute("start");
			unsigned startFrame = 0;
			unsigned endFrame = 0;
			if (sz)
			{
				startFrame = StringConverter::ParseUnsignedInt(sz);
			}

			sz = action->Attribute("end");
			if (sz)
			{
				endFrame = StringConverter::ParseUnsignedInt(sz);
			}

			newAction.mStartTime = startFrame / 24.0f;
			newAction.mEndTime = endFrame / 24.0f;
			newAction.mLength = newAction.mEndTime - newAction.mStartTime;
			newAction.mPosStartEnd[0] = FindPos(newAction.mStartTime);
			newAction.mPosStartEnd[1] = FindPos(newAction.mEndTime);
			newAction.mRotStartEnd[0] = FindRot(newAction.mStartTime);
			newAction.mRotStartEnd[1] = FindRot(newAction.mEndTime);

			sz = action->Attribute("loop");
			if (sz)
				newAction.mLoop = StringConverter::ParseBool(sz);
			action = action->NextSiblingElement("Action");
		}
		return true;
	}

	const Action* GetAction(const char* name) const{
		auto it = mActions.Find(name);
		if (it != mActions.end())
			return &(it->second);
		return 0;
	}

	void GenerateQuatFromEuler(){
		for (const auto& it : mEuler)
		{
			mRot[it.first] = Quat(it.second);
		}
		mEuler.clear();
	}

	const Vec3* FindPos(float time){
		const Vec3* mostClose = 0;
		for (const auto& it : mPos)
		{
			if (IsEqual(it.first, time, 0.01f))
			{
				return &it.second;
			}
			else
			{
				if (it.first < time)
					mostClose = &it.second;
			}
		}
		return mostClose;
	}

	const Vec3* FindScale(float time){
		const Vec3* mostClose = 0;
		for (const auto& it : mScale)
		{
			if (IsEqual(it.first, time, 0.01f))
			{
				return &it.second;
			}
			else
			{
				if (it.first < time)
					mostClose = &it.second;
			}
		}
		return mostClose;
	}

	const Quat* FindRot(float time){
		const Quat* mostClose = 0;
		for (const auto& it : mRot)
		{
			if (IsEqual(it.first, time, 0.01f))
			{
				return &it.second;
			}
			else
			{
				if (it.first < time)
					mostClose = &it.second;
			}
		}
		return mostClose;
	}

	const Vec3* FindRotEuler(float time){
		const Vec3* mostClose = 0;
		for (const auto& it : mEuler)
		{
			if (IsEqual(it.first, time, 0.01f))
			{
				return &it.second;
			}
			else
			{
				if (it.first < time)
					mostClose = &it.second;
			}
		}
		return mostClose;
	}
	
};

//---------------------------------------------------------------------------
AnimationData::Action::Action(){
	mPosStartEnd[0] = mPosStartEnd[1] = 0;
	mRotStartEnd[0] = mRotStartEnd[1] = 0;
	mLength = 0.f;
	mStartTime = mEndTime = 0.f;
	mLoop = false;
}

//---------------------------------------------------------------------------
IMPLEMENT_STATIC_CREATE(AnimationData);
AnimationData::AnimationData()
	: mImpl(new Impl){

}
AnimationData::~AnimationData(){

}

void AnimationData::AddPosition(float time, float v, PosComp comp){
	mImpl->AddPosition(time, v, comp);
}

void AnimationData::AddScale(float time, float v, PosComp comp){
	mImpl->AddScale(time, v, comp);
}

void AnimationData::AddRotEuler(float time, float v, PosComp comp){
	mImpl->AddRotEuler(time, v, comp);
}

bool AnimationData::HasPosAnimation() const{
	return mImpl->HasPosAnimation();
}

bool AnimationData::HasRotAnimation() const{
	return mImpl->HasRotAnimation();
}

bool AnimationData::HasScaleAnimation() const{
	return mImpl->HasScaleAnimation();
}

void AnimationData::SetName(const char* name){
	mImpl->SetName(name);
}

const char* AnimationData::GetName() const{
	return mImpl->GetName();
}

void AnimationData::PickPos(float time, bool cycled, const Vec3** prev, const Vec3** next, float& interpol){
	mImpl->PickPos(time, cycled, prev, next, interpol);
}

void AnimationData::PickRot(float time, bool cycled, const Quat** prev, const Quat** next, float& interpol){
	mImpl->PickRot(time, cycled, prev, next, interpol);
}

void AnimationData::ToLocal(const Transformation& tolocal){
	mImpl->ToLocal(tolocal);
}

bool AnimationData::ParseAction(const char* filename){
	return mImpl->ParseAction(filename);
}

AnimationData::Action* AnimationData::GetAction(const char* name) const{
	return mImpl->GetAction(name);
}