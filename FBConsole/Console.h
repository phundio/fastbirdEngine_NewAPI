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
#include "FBCommonHeaders/Types.h"
#include "FBCommonHeaders/Observable.h"
#include "FBInputManager/IInputConsumer.h"
#include "ICVarObserver.h"
#include "ConsoleDataType.h"
namespace fastbird{
	class StdOutRedirect;
	FB_DECLARE_SMART_PTR(Console);
	class FB_DLL_CONSOLE Console : public IInputConsumer, public Observable<ICVarObserver>{
		FB_DECLARE_PIMPL_NON_COPYABLE(Console);
		Console();		

	public:
		static ConsolePtr Create();
		static Console& GetInstance();		
		static bool HasInstance();

		void SetRenderTargetSize(const Vec2I& size);
		void RegisterCommand(ConsoleCommand* pCom);
		void UnregisterCommand(ConsoleCommand* pCom);
		void RegisterVariable(CVar* cvar);
		void UnregisterVariable(CVar* cvar);
		void AddCandidatesTo(const char* parent, const StringVector& candidates);
		void Log(const char* szFmt, ...);
		void ProcessCommand(const char* command, bool history = true);
		void QueueProcessCommand(const char* command, bool history = true);
		void ToggleOpen();
		void Update();
		void Render();
		void RegisterStdout(StdOutRedirect* p);
		void Clear();

		//---------------------------------------------------------------------------
		// IInputConsumer
		//---------------------------------------------------------------------------
		void ConsumeInput(IInputInjectorPtr injector);
	};
}