#pragma once
#include "KeyboardEvent.h"
#include "MouseEvent.h"
#include "FBInputDevice.h"
#include "FBTimerLib/Timer.h"
namespace fastbird{
	class IInputConsumer;
	DECLARE_SMART_PTR(IMouse);
	DECLARE_SMART_PTR(IKeyboard);
	DECLARE_SMART_PTR(IInputInjector);
	/** Handles user input. 
	You can register your objects which need to receive user input, 
	inherit the object from IInputConsumer and register it to 
	this manager.The object you registered should be Unregistered 
	before it destroyed.
	\ingroup FBInputManager
	*/
	class FB_DLL_PUBLIC InputManager{
		DECLARE_PIMPL(InputManager);
		InputManager();
		~InputManager();
		static InputManager* sInputManager;
	public:

		static InputManager* CreateInputManager();
		static InputManager* GetInstance();
		static void DeleteInputManager();

		//-------------------------------------------------------------------
		// Manager
		//-------------------------------------------------------------------
		void PrepareKeyboard(); /// for default keyboard
		void PrepareMouse(); /// for default mouse
		void SetKeyboard(IKeyboardPtr keyboard);
		void SetMouse(IMousePtr mouse);

		/** Register an input consumer.
		You need unregister when the consumer is destroyed or does not 
		need to getinput information any more.
		\param consumer
		\param priority number one priority is handled first. 
		i.e. the lowest value is handled first. Check the default 
		priority IInputConsumer::Priority
		*/
		void RegisterInputConsumer(IInputConsumer* consumer, int priority);
		void UnregisterInputConsumer(IInputConsumer* consumer, int priority);

		/** Will send input injector to every consumers
		*/
		void Update();

		void Invalidate(FBInputDevice::Enum type, bool includeButtonClicks = false);
		void InvalidTemporary(FBInputDevice::Enum type, bool invalidate);
		bool IsValid(FBInputDevice::Enum type) const;
		void EndFrame(Timer::TIME_PRECISION gameTimeInSecond);

		//-------------------------------------------------------------------
		// Common
		//-------------------------------------------------------------------
		void OnSetFocus(HWindow hWnd);
		void OnKillFocus();
		void AddHwndInterested(HWindow wnd);
		void SetInputInjector(IInputInjectorPtr injector);

		//-------------------------------------------------------------------
		// Keyboard
		//-------------------------------------------------------------------
		void PushKeyEvent(HWindow hWnd, const KeyboardEvent& keyboardEvent);
		void PushChar(HWindow hWnd, unsigned keycode, Timer::TIME_PRECISION gameTimeInSec);
		

		//-------------------------------------------------------------------
		// Mouse
		//-------------------------------------------------------------------
		void PushMouseEvent(HWindow handle, const MouseEvent& mouseEvent, Timer::TIME_PRECISION);
		Real GetSensitivity() const;
		void SetSensitivity(Real sens);
		Real GetWheelSensitivity() const;
		void SetWheelSensitivity(Real sens);
		void SetMousePosition(int x, int y);
		void MouseToCenter();
	};
}