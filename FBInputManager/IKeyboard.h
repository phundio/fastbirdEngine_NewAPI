#pragma once
#ifndef _fastbird_IKeyboard_header_included_
#define _fastbird_IKeyboard_header_included_

#include "IInputDevice.h"
#include "KeyboardEvent.h"
namespace fastbird
{
	DECLARE_SMART_PTR(IKeyboard);	
	class IKeyboard : public IInputDevice
	{
	public:
		enum KEYBOARD_FLAG
		{
			KEYBOARD_FLAG_KEY_BREAK = 0x01,
			KEYBOARD_FLAG_KEY_E0 = 0x02,
			KEYBOARD_FLAG_KEY_E1 = 0x04,
			KEYBOARD_FLAG_KEY_MAKE = 0,
		};
		virtual ~IKeyboard(){}
		virtual void PushEvent(HWindow hWnd, const KeyboardEvent& keyboardEvent) = 0;
		virtual void PushChar(HWindow hWnd, unsigned keycode, Timer::TIME_PRECISION gameTimeInSec) = 0;
		virtual bool IsKeyDown(unsigned short keycode) const = 0;
		virtual bool IsKeyPressed(unsigned short keycode) const = 0;
		virtual bool IsKeyUp(unsigned short keycode) const = 0;
		virtual unsigned GetChar() = 0;
		virtual void PopChar() = 0;
		virtual void OnKillFocus() = 0;
		virtual void ClearBuffer() = 0;
	};
}


#endif //_fastbird_IKeyboard_header_included_