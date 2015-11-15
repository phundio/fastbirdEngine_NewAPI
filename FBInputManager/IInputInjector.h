#pragma once
#define FB_INPUT_INJECTOR_VERSION 1
#include "FBInputDevice.h"
#include "FBCommonHeaders/Types.h"
namespace fastbird{
	DECLARE_SMART_PTR(IKeyboard);
	DECLARE_SMART_PTR(IMouse);
	DECLARE_SMART_PTR(IInputInjector);
	class IInputInjector{
	public:
		virtual ~IInputInjector(){}
		
		virtual bool IsValid(FBInputDevice::Enum type) const = 0;
		virtual void Invalidate(FBInputDevice::Enum type) const = 0;
		virtual void InvalidTemporary(FBInputDevice::Enum type, bool invalidate) = 0;

		//-------------------------------------------------------------------
		// Keyboard
		//-------------------------------------------------------------------
		virtual bool IsKeyDown(unsigned short keycode) const = 0;
		virtual bool IsKeyPressed(unsigned short keycode) const = 0;
		virtual bool IsKeyUp(unsigned short keycode) const = 0;
		virtual unsigned GetChar() = 0;
		virtual void PopChar() = 0;
		virtual void ClearBuffer() = 0;		

		//-------------------------------------------------------------------
		// Mouse
		//-------------------------------------------------------------------
		// Positions
		virtual void GetHDDeltaXY(long &x, long &y) const = 0;
		virtual void GetDeltaXY(long &x, long &y) const = 0;
		virtual Vec2ITuple GetDeltaXY() const = 0;
		virtual void GetMousePos(long &x, long &y) const = 0;
		virtual Vec2ITuple GetMousePos() const = 0;
		virtual void GetMousePrevPos(long &x, long &y) const = 0;
		virtual void GetMouseNPos(Real &x, Real &y) const = 0; // normalized pos(0.0~1.0)
		virtual Vec2Tuple GetMouseNPos() const = 0;
		virtual bool IsMouseMoved() const = 0;
		virtual void LockMousePos(bool lock, void* key) = 0;
		virtual bool IsMouseIn(int left, int top, int right, int bottom) = 0;
		virtual Real GetSensitivity() const = 0;

		// Dragging
		virtual void GetDragStart(long &x, long &y) const = 0;
		virtual Vec2ITuple GetDragStartedPos() const = 0;
		virtual bool IsDragStartIn(int left, int top, int right, int bottom) const = 0;
		virtual bool IsDragStarted(int& outX, int& outY) const = 0;
		virtual bool IsDragEnded() const = 0;
		virtual void PopDragEvent() = 0;
		virtual bool IsRDragStarted(int& outX, int& outY) const = 0;
		virtual bool IsRDragEnded(int& outX, int& outY) const = 0;
		virtual void PopRDragEvent() = 0;

		// Buttons
		virtual bool IsLButtonDownPrev() const = 0;
		virtual bool IsLButtonDown(Real* time = 0) const = 0;
		virtual bool IsLButtonClicked() const = 0;
		virtual bool IsLButtonDoubleClicked() const = 0;
		virtual bool IsLButtonPressed() const = 0;
		virtual bool IsRButtonDown(Real* time = 0) const = 0;
		virtual bool IsRButtonDownPrev() const = 0;
		virtual bool IsRButtonClicked() const = 0;
		virtual bool IsRButtonPressed() const = 0;
		virtual bool IsMButtonDown() const = 0;
		virtual void ClearButton() = 0;
		virtual void NoClickOnce() = 0;
		virtual void ClearRightDown() = 0;
		virtual Real GetLButtonDownTime() const = 0;

		// Wheel		
		virtual long GetWheel() const = 0;
		virtual void PopWheel() = 0;
		virtual void ClearWheel() = 0;				
		virtual Real GetWheelSensitivity() const = 0;
		virtual unsigned long GetNumLinesWheelScroll() const = 0;

	private:
		friend class InputManager;
		virtual void SetKeyboard(IKeyboardPtr keyboard) = 0;
		virtual void SetMouse(IMousePtr mouse) = 0;
	};
}