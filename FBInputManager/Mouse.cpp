#include "stdafx.h"
#include "Mouse.h"
#include "FBCommonHeaders/Helpers.h"

namespace fastbird
{
static int Round(Real v)
{
	return (int)(v + 0.5f);
}
class Mouse::MouseImpl{
public:
	HWindow mLastEventWindow;
	int mButtonsDown;
	int mButtonsDownPrev;
	int mButtonsPressed;
	int mButtonsClicked;
	int mButtonsDoubleClicked;

	// this should be used for picking.
	long mAbsX;
	long mAbsXPrev;
	long mAbsY;
	long mAbsYPrev;

	long mPhysicalX;
	long mPhysicalY;

	Real mNPosX;
	Real mNPosY;

	// this is good to use camera rotation.
	long mLastX;
	long mLastY;

	long mDragStartX;
	long mDragStartY;
	long mDragEndX;
	long mDragEndY;

	std::stack<long> mWheel;

	bool mValid;
	unsigned long mNumLinesWheelScroll;

	double mDoubleClickSpeed;
	bool mLButtonDoubleClicked;
	bool mLockMouse;

	CoordinatesI mLastDownPos;
	Real mLastLeftDownTime;
	Real mLastRightDownTime;
	Real mLastUpTime;
	CoordinatesI mLastClickPos;
	Real mLastClickTime;

	Real mLastWheelPush;
	bool mDragStarted;
	bool mDragEnd;

	bool mRDragStarted;
	bool mRDragEnd;
	long mRDragStartX;
	long mRDragStartY;
	long mRDragEndX;
	long mRDragEndY;

	void* mLockMouseKey;
	bool mInvalidatedTemporary;
	bool mNoClickOnce;
	std::map<HWindow, CoordinatesI> mRenderTargetSizes;
	std::vector<HWindow> mInterestedWindows;
	Real mSensitivity;
	Real mWheelSensitivity;

	MouseImpl() 
		: mLastDownPos(0, 0)
		, mLastLeftDownTime(0)
		, mLastRightDownTime(0)
		, mLastClickTime(0)
		, mLastClickPos(0, 0)
		, mLockMouse(false)
		, mNPosX(0)
		, mNPosY(0)
		, mPhysicalX(0)
		, mPhysicalY(0)		
		, mLastWheelPush(0)
		, mDragStarted(false)
		, mDragEnd(false)
		, mLockMouseKey(0), mInvalidatedTemporary(false)
		, mRDragStarted(false)
		, mRDragEnd(false)
		, mNoClickOnce(false)
		, mSensitivity(0.03)
		, mWheelSensitivity(0.005){

		mLButtonDoubleClicked = false;
		mButtonsDown = 0;
		mButtonsDownPrev = 0;
		mButtonsClicked = 0;
		mButtonsDoubleClicked = 0;
		GetCurrentMousePos(mAbsX, mAbsY, mPhysicalX, mPhysicalY);
		mAbsXPrev = mAbsX;
		mAbsYPrev = mAbsY;
		mLastX = 0;
		mLastY = 0;
		mValid = true;
		mNumLinesWheelScroll = 0;
#if defined(_PLATFORM_WINDOWS_)
		SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &mNumLinesWheelScroll, 0);
#else
		assert(0 && "Not implemented");
#endif
		ClearDrag();
		mDoubleClickSpeed = (Real)GetDoubleClickTime() / 1000.0f;
	}

	//-------------------------------------------------------------------
	// IInputDevice
	//-------------------------------------------------------------------
	void EndFrame(Real gameTimeInSec){
		mButtonsClicked = 0;
		mButtonsPressed = 0;
		mLastX = 0;
		mLastY = 0;
		if (mLockMouse){
			// mLastEventWindow is ok?
			// or need the current forground window.
			SetCurrentMousePos(mAbsXPrev, mAbsYPrev);
		}
		else{
			mAbsXPrev = mAbsX;
			mAbsYPrev = mAbsY;
		}
		mValid = true;
		mLButtonDoubleClicked = false;
		if (gameTimeInSec - mLastWheelPush > 0.5f){
			while (!mWheel.empty()){
				mWheel.pop();
			}
		}
	}

	bool IsValid() const {
		return mValid && !mInvalidatedTemporary;
	}

	void Invalidate(bool buttonClicked = false){
		if (buttonClicked)
			mLastClickTime = 0;
		mValid = false;
	}

	void InvalidTemporary(bool invalidate){
		mInvalidatedTemporary = invalidate;
	}

	//-------------------------------------------------------------------
	// IMouse
	//-------------------------------------------------------------------
	void PushEvent(HWindow handle, const MouseEvent& mouseEvent, Timer::TIME_PRECISION gameTimeInSec){
		/*DebugOutput("usFlags = %x, usButtonFlags = %x, usButtonData = %x, ulRawButtons = %x, lLastX = %d, lLastY = %d, ulExtraInformation = %d",
		mouseEvent.usFlags,
		mouseEvent.usButtonFlags,
		mouseEvent.usButtonData,
		mouseEvent.ulRawButtons,
		mouseEvent.lLastX,
		mouseEvent.lLastY,
		mouseEvent.ulExtraInformation);*/

		if (!mLockMouse)
		{
			GetCurrentMousePos(mAbsX, mAbsY, mPhysicalX, mPhysicalY);
			const auto& size = mRenderTargetSizes[handle];
			mNPosX = (Real)mAbsX / (Real)size.first;
			mNPosY = (Real)mAbsY / (Real)size.second;
			if (!IsLButtonDown()){
				auto hwnd = FBWindowFromPoint(mPhysicalX, mPhysicalY);
				if (hwnd != handle)
				{
					if (ValueExistsInVector(mInterestedWindows, hwnd)){
						FBSetForegroundWindow(hwnd);
					}
				}
			}
		}


		mLastX = mouseEvent.lLastX;
		mLastY = mouseEvent.lLastY;

		static HWindow downHwnd = 0;
		mButtonsDownPrev = mButtonsDown;
		if (mouseEvent.usButtonFlags & MOUSE_BUTTON_FLAG_LEFT_BUTTON_DOWN)
		{
			mDragEnd = false;
			if ((mButtonsDown & MOUSE_BUTTON_LEFT) == 0)
			{
				// drag
				mDragStartX = mAbsX;
				mDragStartY = mAbsY;
				downHwnd = FBGetForegroundWindow();
			}
			mButtonsDown |= MOUSE_BUTTON_LEFT;
			mButtonsPressed |= MOUSE_BUTTON_LEFT;

			mLastLeftDownTime = gameTimeInSec;
			mLastDownPos.first = mAbsX;
			mLastDownPos.second = mAbsY;
		}
		if (IsLButtonDown() && !mDragStarted)
		{
			if (std::abs(mDragStartX - mAbsX) > 3 || std::abs(mDragStartY - mAbsY) > 3)
				mDragStarted = true;
		}

		if (mouseEvent.usButtonFlags & MOUSE_BUTTON_FLAG_RIGHT_BUTTON_DOWN)
		{
			mRDragEnd = false;
			if ((mButtonsDown & MOUSE_BUTTON_RIGHT) == 0)
			{
				// drag
				mRDragStartX = mAbsX;
				mRDragStartY = mAbsY;
			}

			mButtonsDown |= MOUSE_BUTTON_RIGHT;
			mButtonsPressed |= MOUSE_BUTTON_RIGHT;

			mLastRightDownTime = gameTimeInSec;
			mLastDownPos.first = mAbsX;
			mLastDownPos.second = mAbsY;
		}
		if (IsRButtonDown() && !mRDragStarted)
		{
			if (std::abs(mRDragStartX - mAbsX) > 3 || std::abs(mRDragStartY - mAbsY) > 3)
				mRDragStarted = true;
		}
		if (mouseEvent.usButtonFlags & MOUSE_BUTTON_FLAG_MIDDLE_BUTTON_DOWN)
		{
			mButtonsDown |= MOUSE_BUTTON_MIDDLE;
			mButtonsPressed |= MOUSE_BUTTON_MIDDLE;

			//mLastDownTime = gFBEnv->pTimer->GetTime();
			mLastDownPos.first = mAbsX;
			mLastDownPos.second = mAbsY;
		}
		if (mouseEvent.usButtonFlags & MOUSE_BUTTON_FLAG_BUTTON_4_DOWN)
		{
			mButtonsDown |= MOUSE_BUTTON_4;
			mButtonsPressed |= MOUSE_BUTTON_4;

			//mLastDownTime = gFBEnv->pTimer->GetTime();
			mLastDownPos.first = mAbsX;
			mLastDownPos.second = mAbsY;
		}
		if (mouseEvent.usButtonFlags & MOUSE_BUTTON_FLAG_BUTTON_5_DOWN)
		{
			mButtonsDown |= MOUSE_BUTTON_5;
			mButtonsPressed |= MOUSE_BUTTON_5;

			//mLastDownTime = gFBEnv->pTimer->GetTime();
			mLastDownPos.first = mAbsX;
			mLastDownPos.second = mAbsY;
		}


		bool mouseNotMoved = abs((mLastDownPos.first - mAbsX)) < 4 && abs((mLastDownPos.second - mAbsY)) < 4;

		auto curTime = gameTimeInSec;
		auto leftElapsedTime = (curTime - mLastLeftDownTime);
		auto rightElapsedTime = (curTime - mLastRightDownTime);
		if (mouseEvent.usButtonFlags & MOUSE_BUTTON_FLAG_LEFT_BUTTON_UP)
		{
			if (mButtonsDown & MOUSE_BUTTON_LEFT)
			{
				mDragEndX = mAbsX;
				mDragEndY = mAbsY;
				if (mDragStarted){
					mDragStarted = false;
				}
				mDragEnd = true;
				if (mDragStartX == mDragEndX && mDragStartY == mDragEndY)
				{
					ClearDrag();
				}
			}
			auto curHwnd = FBWindowFromPoint(mPhysicalX, mPhysicalY);
			if (curHwnd != downHwnd){
				Invalidate();
				EndFrame(0);
			}
			else{
				mButtonsDown &= ~MOUSE_BUTTON_LEFT;

				Real doubleClickElapsedTime = curTime - mLastClickTime;
				bool doubleClickMouseNotMoved = abs((mLastClickPos.first - mAbsX)) < 6 && abs((mLastClickPos.second - mAbsY)) < 6;
				if (doubleClickElapsedTime < mDoubleClickSpeed && doubleClickMouseNotMoved)
				{
					mLButtonDoubleClicked = true;
				}

				//if (mouseNotMoved && !mLButtonDoubleClicked && leftElapsedTime < 0.25f)
				if (!mLButtonDoubleClicked)
				{
					if (mNoClickOnce){
						mNoClickOnce = false;
					}
					else{
						mButtonsClicked |= MOUSE_BUTTON_LEFT;
						mLastClickTime = gameTimeInSec;
						mLastClickPos = CoordinatesI(mAbsX, mAbsY);
					}
				}
			}

			LockMousePos(false, (void*)-1);
		}
		if (mouseEvent.usButtonFlags & MOUSE_BUTTON_FLAG_RIGHT_BUTTON_UP)
		{
			mRDragEndX = mAbsX;
			mRDragEndY = mAbsY;
			if (mRDragStarted){
				mRDragStarted = false;
			}
			mRDragEnd = true;
			if (mRDragStartX == mRDragEndX && mRDragStartY == mRDragEndY)
			{
				ClearRDrag();
			}

			mButtonsDown &= ~MOUSE_BUTTON_RIGHT;


			if (mNoClickOnce){
				mNoClickOnce = false;
			}
			else{
				mButtonsClicked |= MOUSE_BUTTON_RIGHT;
				mLastClickTime = gameTimeInSec;
				mLastClickPos = { mAbsX, mAbsY };
			}

			LockMousePos(false, (void*)-1);
		}
		if (mouseEvent.usButtonFlags & MOUSE_BUTTON_FLAG_MIDDLE_BUTTON_UP)
		{
			mButtonsDown &= ~MOUSE_BUTTON_MIDDLE;
			mButtonsClicked |= MOUSE_BUTTON_MIDDLE;
			mLastClickTime = gameTimeInSec;
			mLastClickPos = { mAbsX, mAbsY };
		}
		if (mouseEvent.usButtonFlags & MOUSE_BUTTON_FLAG_BUTTON_4_UP)
		{
			mButtonsDown &= ~MOUSE_BUTTON_4;
			if (mouseNotMoved)
			{
				mButtonsClicked |= MOUSE_BUTTON_4;
				mLastClickTime = gameTimeInSec;
				mLastClickPos = { mAbsX, mAbsY };
			}
		}
		if (mouseEvent.usButtonFlags & MOUSE_BUTTON_FLAG_BUTTON_5_UP)
		{
			mButtonsDown &= ~MOUSE_BUTTON_5;
			if (mouseNotMoved)
			{
				mButtonsClicked |= MOUSE_BUTTON_5;
				mLastClickTime = gameTimeInSec;
				mLastClickPos = { mAbsX, mAbsY };
			}
		}

		if (mouseEvent.usButtonFlags & MOUSE_BUTTON_FLAG_MOUSE_WHEEL)
		{
			mWheel.push((short)mouseEvent.usButtonData);
			mLastWheelPush = gpTimer->GetTime();
		}
	}

	void GetHDDeltaXY(long &x, long &y) const{
		x = mLastX;
		y = mLastY;
	}

	void GetDeltaXY(long &x, long &y) const{
		x = mAbsX - mAbsXPrev;
		y = mAbsY - mAbsYPrev;
	}

	CoordinatesI GetDeltaXY() const{
		return CoordinatesI(mAbsX - mAbsXPrev, mAbsY - mAbsYPrev);
	}

	void GetPos(long &x, long &y) const{
		x = mAbsX;
		y = mAbsY;
	}

	CoordinatesI GetPos() const{
		return CoordinatesI(mAbsX, mAbsY);
	}

	void GetPrevPos(long &x, long &y) const{
		x = mAbsXPrev;
		y = mAbsYPrev;
	}

	void GetNPos(Real &x, Real &y) const{
		x = mNPosX;
		y = mNPosY;
	}

	CoordinatesR GetNPos() const{
		return{ mNPosX, mNPosY };
	}

	bool IsLButtonDownPrev() const{
		return (mButtonsDownPrev&MOUSE_BUTTON_LEFT) != 0;
	}

	bool IsLButtonDown(Real* time = 0) const{
		if (time)
			*time = mLastLeftDownTime;
		return (mButtonsDown & MOUSE_BUTTON_LEFT) != 0;
	}

	bool IsLButtonClicked() const{
		return (mButtonsClicked & MOUSE_BUTTON_LEFT) != 0 && !mLButtonDoubleClicked;
	}

	bool IsLButtonDoubleClicked() const{
		return mLButtonDoubleClicked;
	}

	bool IsLButtonPressed() const{
		return (mButtonsPressed&MOUSE_BUTTON_LEFT) != 0;
	}

	bool IsRButtonDown(Real* time = 0) const{
		if (time)
			*time = mLastRightDownTime;
		return (mButtonsDown & MOUSE_BUTTON_RIGHT) != 0;
	}

	bool IsRButtonDownPrev() const{
		return (mButtonsDownPrev & MOUSE_BUTTON_RIGHT) != 0;
	}

	bool IsRButtonClicked() const{
		return (mButtonsClicked & MOUSE_BUTTON_RIGHT) != 0;
	}

	bool IsRButtonPressed() const{
		return (mButtonsPressed&MOUSE_BUTTON_RIGHT) != 0;
	}

	bool IsMButtonDown() const{
		return (mButtonsDown&MOUSE_BUTTON_MIDDLE) != 0;
	}

	bool IsMoved() const{
		return mLastX != 0 || mLastY != 0;
	}

	void GetDragStart(long &x, long &y) const{
		x = mDragStartX;
		y = mDragStartY;
	}

	CoordinatesI GetDragStartedPos() const{
		return{ mDragStartX, mDragStartY };
	}

	bool IsDragStartIn(int left, int top, int right, int bottom) const{
		if (mDragStartX < left ||
			mDragStartX > right ||
			mDragStartY < top ||
			mDragStartY > bottom)
			return false;

		return true;

	}

	bool IsDragStarted(int& outX, int& outY) const{
		if (mDragStarted)
		{
			outX = mDragStartX;
			outY = mDragStartY;
		}
		return mDragStarted;
	}

	bool IsDragEnded() const{
		return mDragEnd;
	}

	void PopDragEvent(){
		mDragStarted = false;
		mDragEnd = false;
	}

	bool IsRDragStarted(int& outX, int& outY) const{
		if (mRDragStarted){
			outX = mRDragStartX;
			outY = mRDragStartY;
		}
		return mRDragStarted;
	}

	bool IsRDragEnded(int& outX, int& outY) const{
		return mRDragEnd;
	}

	void PopRDragEvent(){
		mRDragStarted = false;
		mRDragEnd = false;
	}

	long GetWheel() const{
		if (mWheel.empty())
			return 0;
		return mWheel.top();
	}

	void PopWheel(){
		mWheel.pop();
	}

	void ClearWheel(){
		while (!mWheel.empty())
			mWheel.pop();
	}

	void ClearButton(){
		mButtonsClicked = 0;
		mButtonsPressed = 0;
	}

	unsigned long GetNumLinesWheelScroll() const{
		return mNumLinesWheelScroll;
	}

	void LockMousePos(bool lock, void* key){
		if (mLockMouse == lock)
			return;
		if (!lock && mLockMouseKey != key && key != (void*)-1)
			return;

		mLockMouseKey = key;

		if (lock)
		{
			if (!mInterestedWindows.empty() && FBGetForegroundWindow() == mInterestedWindows[0])
			{
				if (!mLockMouse)
				{
					mAbsXPrev = mAbsX;
					mAbsYPrev = mAbsY;
				}
				mLockMouse = true;
				int displayCounter = ShowCursor(false);
				while (displayCounter >= 0)
				{
					displayCounter = ShowCursor(false);
				}
				return;
			}
		}

		int displayCounter = ShowCursor(true);
		while (displayCounter<0)
		{
			displayCounter = ShowCursor(true);
		}

		mLockMouse = false;
	}

	void NoClickOnce(){
		mNoClickOnce = true;
	}

	void OnKillFocus(){
		LockMousePos(false, (void*)-1);
		mButtonsDown = 0;
	}

	void OnSetFocus(HWindow hWnd){
		GetCurrentMousePos(mAbsX, mAbsY, mPhysicalX, mPhysicalY);
		const auto& size = mRenderTargetSizes[hWnd];			
		mNPosX = (Real)mAbsX / (Real)size.first;
		mNPosY = (Real)mAbsY / (Real)size.second;
		mAbsXPrev = mAbsX;
		mAbsYPrev = mAbsY;

		LockMousePos(false, (void*)-1);
		if (mDragStarted)
		{
			mDragStarted = false;
			mDragEnd = true;
		}
	}

	bool IsIn(int left, int top, int right, int bottom){
		return !(mAbsX < left || mAbsX > right || mAbsY < top || mAbsY > bottom);
	}

	void CursorToCenter(){
		if (IsMainWindowForeground()){			
			auto size = GetForegroundRenderTargetSize();
			size = { Round(size.first / 2) , Round(size.second / 2) };
			SetCurrentMousePos(size.first, size.second);
			mAbsX = size.first;
			mAbsY = size.second;
			mAbsXPrev = size.first;
			mAbsYPrev = size.second;
		}
	}

	void SetCursorPosition(int x, int y){
		SetCurrentMousePos(x, y);
		mAbsX = x;
		mAbsY = y;
		mAbsXPrev = x;
		mAbsYPrev = y;
	}

	void ClearRightDown(){
		mButtonsDown &= ~MOUSE_BUTTON_RIGHT;
	}

	Real GetLButtonDownTime() const{
		return mLastLeftDownTime;
	}

	void AddHwndInterested(HWindow wnd){
		mInterestedWindows.push_back(wnd);
	}

	void OnRenderTargetSizeChanged(int x, int y, HWindow associatedWindow){
		mRenderTargetSizes[associatedWindow] = CoordinatesI(x, y);
		if (FBGetForegroundWindow() == associatedWindow){
			mNPosX = mAbsX / (Real)x;
			mNPosY = mAbsY / (Real)y;
		}
	}

	void GetCurrentMousePos(long& x, long& y, long& physicalX, long& physicalY)
	{
#ifdef _PLATFORM_WINDOWS_
		auto rtSize = GetForegroundRenderTargetSize();		
		CoordinatesI windowSize = GetForgroundWindowSize();

		POINT cursor;
		GetCursorPos(&cursor);
		physicalX = cursor.x;
		physicalY = cursor.y;		
		ScreenToClient(GetForegroundWindow(), &cursor);
		x = cursor.x;
		y = cursor.y;
		if (windowSize != rtSize){
			Real xr = rtSize.first / (Real)windowSize.first;
			Real yr = rtSize.second / (Real)windowSize.second;
			x = Round(x * xr);
			y = Round(y * yr);
		}
#else
		assert(0 && "Not implemented");
#endif _PLATFORM_WINDOWS_
	}

	void SetCurrentMousePos(long x, long y)
	{
#ifdef _PLATFORM_WINDOWS_
		auto rtSize = GetForegroundRenderTargetSize();
		CoordinatesI windowSize = GetForgroundWindowSize();
		if (windowSize != rtSize){
			Real xr = windowSize.first / (Real)rtSize.first;
			Real yr = windowSize.second / (Real)rtSize.second;
			x = Round(x*xr);
			y = Round(y*yr);
		}

		POINT cursor;
		cursor.x = x;
		cursor.y = y;

		ClientToScreen(GetForegroundWindow(), &cursor);
		SetCursorPos(cursor.x, cursor.y);
#else
		assert(0);
#endif _PLATFORM_WINDOWS_
	}

	void ClearDrag(){		
		mDragStarted = false;
		mDragEnd = true;
	}

	void ClearRDrag(){
		mRDragStartX = mRDragEndX = -1;
		mRDragStartY = mRDragEndY = -1;
		mRDragStarted = false;
		mRDragEnd = true;
	}

	CoordinatesI GetForgroundWindowSize(){
#if defined(_PLATFORM_WINDOWS_)
		auto wnd = GetForegroundWindow();
		RECT rect;
		GetWindowRect(wnd, &rect);
		return CoordinatesI(rect.right - rect.left, rect.bottom - rect.top);
#else
		assert(0 && "Not implemented");
#endif
	}

	CoordinatesI GetForegroundRenderTargetSize(){
		HWindow wnd = (HWindow)GetForegroundWindow();
		return mRenderTargetSizes[wnd];
	}

	HWindow FBGetForegroundWindow(){
#if defined(_PLATFORM_WINDOWS_)
		return (HWindow)GetForegroundWindow();
#else
		assert(0 && "Not implemented");
#endif
	}

	HWindow FBWindowFromPoint(int x, int y){
#if defined(_PLATFORM_WINDOWS_)
		return (HWindow)WindowFromPoint(POINT{ x, y });
#else
		assert(0 && "Not implemented");
#endif
	}

	void FBSetForegroundWindow(HWindow hwnd){
#if defined(_PLATFORM_WINDOWS_)
		SetForegroundWindow((HWND)hwnd);
#else
		assert(0 && "Not Implemented.");
#endif
	}

	bool IsMainWindowForeground(){
		auto foreground = FBGetForegroundWindow();
		if (!mInterestedWindows.empty() && mInterestedWindows[0] == foreground){
			return true;
		}
		return false;
	}
};

Mouse::Mouse()
	: mImpl(new MouseImpl){		
}
Mouse::~Mouse(){
	delete mImpl;
}	

void Mouse::PushEvent(HWindow handle, const MouseEvent& mouseEvent, Timer::TIME_PRECISION gameTimeInSec)	{
	mImpl->PushEvent(handle, mouseEvent, gameTimeInSec);
}

void Mouse::EndFrame(Timer::TIME_PRECISION gameTimeInSec)	{
	mImpl->EndFrame(gameTimeInSec);
}

bool Mouse::IsValid() const{
	return mImpl->IsValid();
}

void Mouse::Invalidate(bool buttonClicked){
	mImpl->Invalidate(buttonClicked);
}

void Mouse::InvalidTemporary(bool invalidate){
	mImpl->InvalidTemporary(invalidate);
}

//-------------------------------------------------------------------------
void Mouse::GetHDDeltaXY(long &x, long &y) const{
	mImpl->GetHDDeltaXY(x, y);		
}

void Mouse::GetDeltaXY(long &x, long &y) const	{
	mImpl->GetDeltaXY(x, y);
}

CoordinatesI Mouse::GetDeltaXY() const{
	return mImpl->GetDeltaXY();
}

void Mouse::GetPos(long &x, long &y) const{
	mImpl->GetPos(x, y);
}

CoordinatesI Mouse::GetPos() const{
	return mImpl->GetPos();
}

void Mouse::GetPrevPos(long &x, long &y) const
{
	mImpl->GetPrevPos(x, y);
}

void Mouse::GetNPos(Real &x, Real &y) const{
	mImpl->GetNPos(x, y);
}

void Mouse::GetDragStart(long &x, long &y) const	{
	mImpl->GetDragStart(x, y);
}

CoordinatesI Mouse::GetDragStartedPos() const{
	return mImpl->GetDragStartedPos();
}

CoordinatesR Mouse::GetNPos() const{
	return mImpl->GetNPos();
}

//-------------------------------------------------------------------------
// LButton
//-------------------------------------------------------------------------

bool Mouse::IsLButtonDownPrev() const{
	return mImpl->IsLButtonDownPrev();
}

bool Mouse::IsLButtonDown(Real* time) const{
	return mImpl->IsLButtonDown(time);		
}
	
bool Mouse::IsLButtonClicked() const{
	return mImpl->IsLButtonClicked();
}
bool Mouse::IsLButtonDoubleClicked() const{
	return mImpl->IsLButtonDoubleClicked();
}

bool Mouse::IsLButtonPressed() const{
	return mImpl->IsLButtonPressed();
}

bool Mouse::IsMButtonDown() const{
	return mImpl->IsMButtonDown();
}

//-------------------------------------------------------------------------
// RButton
//-------------------------------------------------------------------------
bool Mouse::IsRButtonDown(Real* time) const{
	return mImpl->IsRButtonDown(time);
}

bool Mouse::IsRButtonDownPrev() const{
	return mImpl->IsRButtonDownPrev();
}

bool Mouse::IsRButtonClicked() const{
	return mImpl->IsRButtonClicked();
}

bool Mouse::IsRButtonPressed() const{
	return mImpl->IsRButtonPressed();
}

bool Mouse::IsMoved() const{
	return mImpl->IsMoved();
}

bool Mouse::IsDragStartIn(int left, int top, int right, int bottom) const{
	return mImpl->IsDragStartIn(left, top, right, bottom);
}

bool Mouse::IsDragStarted(int& outX, int& outY) const{
	return mImpl->IsDragStarted(outX, outY);
}

bool Mouse::IsDragEnded() const{
	return mImpl->IsDragEnded();
}

void Mouse::PopDragEvent(){
	mImpl->PopDragEvent();
}

bool Mouse::IsRDragStarted(int& outX, int& outY) const{
	return mImpl->IsRDragStarted(outX, outY);
}

bool Mouse::IsRDragEnded(int& outX, int& outY) const{
	return mImpl->IsRDragEnded(outX, outY);
}

void Mouse::PopRDragEvent(){
	mImpl->PopRDragEvent();
}

long Mouse::GetWheel() const
{
	return mImpl->GetWheel();	
}
	
void Mouse::PopWheel(){
	mImpl->PopWheel();
}

void Mouse::ClearWheel(){
	mImpl->ClearWheel();
}

void Mouse::ClearButton(){
	mImpl->ClearButton();
}

unsigned long Mouse::GetNumLinesWheelScroll() const{
	return mImpl->GetNumLinesWheelScroll();
}

void Mouse::LockMousePos(bool lock, void* key){
	mImpl->LockMousePos(lock, key);
}

void Mouse::OnSetFocus(HWindow hWnd){
	mImpl->OnSetFocus(hWnd);
}

void Mouse::OnKillFocus(){		
	mImpl->OnKillFocus();
}

bool Mouse::IsIn(int left, int top, int right, int bottom){
	return mImpl->IsIn(left, top, right, bottom);
}

void Mouse::CursorToCenter(){
	mImpl->CursorToCenter();
}

void Mouse::SetCursorPosition(int x, int y){
	mImpl->SetCurrentMousePos(x, y);
}

void Mouse::NoClickOnce(){
	mImpl->NoClickOnce();
}

void Mouse::ClearRightDown(){		
	mImpl->ClearRightDown();
}

Real Mouse::GetLButtonDownTime() const{
	return mImpl->GetLButtonDownTime();
}

void Mouse::AddHwndInterested(HWindow wnd){
	mImpl->AddHwndInterested(wnd);
}

Real Mouse::GetSensitivity() const{
	return mImpl->mSensitivity;
}

void Mouse::SetSensitivity(Real sens){
	mImpl->mSensitivity = sens;
}

Real Mouse::GetWheelSensitivity() const{
	return mImpl->mWheelSensitivity;
}

void Mouse::SetWheelSensitivity(Real sens){
	mImpl->mWheelSensitivity = sens;
}

void Mouse::OnRenderTargetSizeChanged(int x, int y, HWindow associatedWindow){
	mImpl->OnRenderTargetSizeChanged(x, y, associatedWindow);
}

}