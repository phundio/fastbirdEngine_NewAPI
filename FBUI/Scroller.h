/*
 -----------------------------------------------------------------------------
 This source file is part of fastbird engine
 For the latest info, see http://www.jungwan.net/
 
 Copyright (c) 2013-2015 Jungwan Byun
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 -----------------------------------------------------------------------------
*/

#pragma once
#include "WinBase.h"
namespace fastbird
{

FB_DECLARE_SMART_PTR(Scroller);
class Scroller : public WinBase
{
protected:
	Scroller();
	~Scroller();

public:
	static ScrollerPtr Create();

	// IWinBase
	ComponentType::Enum GetType() const { return ComponentType::Scroller; }
	void GatherVisit(std::vector<UIObject*>& v);
	bool OnInputFromHandler(IInputInjectorPtr injector);

	void OnStartUpdate(float elapsedTime);

	// Own
	void SetMaxOffset(const Vec2& maxOffset);
	const Vec2& GetMaxOffset() const { return mMaxOffset; }
	const Vec2& GetOffset() const { return mOffset; }
	void SetOffset(const Vec2& offset);
	void ResetScroller();

private:
	Vec2 mOffset;
	float mScrollAmount;
	float mScrollAmountScale;
	Vec2 mMaxOffset;	
	float mMaxScrollSpeed;
	float mCurScrollSpeed;
	float mScrollAcc;
	float mCurOffset;
	float mDestOffset;
	int mLastWheel;
	float mLastScrollTime;
	
};
}