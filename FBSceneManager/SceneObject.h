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
#include "SceneObjectFlag.h"
namespace fastbird{
	struct RenderParam;
	struct RenderParamOut;	
	FB_DECLARE_SMART_PTR(Scene);
	FB_DECLARE_SMART_PTR(SceneObject);
	class FB_DLL_SCENEMANAGER SceneObject{
		FB_DECLARE_PIMPL_CLONEABLE(SceneObject);				

	protected:
		SceneObject();
		~SceneObject();


	public:
		void SetName(const char* name);
		const char* GetName() const;		
		void OnAttachedToScene(ScenePtr pScene);
		void OnDetachedFromScene(ScenePtr pScene);
		
		bool IsAttached() const;
		/**
		\param pScene Pass null(Same with calling IsAttached()) to check whether the object is attached to any scene. Otherwise checks for the specified scene.
		*/
		bool IsAttached(ScenePtr pScene) const;

		//-------------------------------------------------------------------
		// Object Flags
		//-------------------------------------------------------------------
		/// Combination of SceneObjectFlag::Enum
		void SetObjFlag(unsigned flag);
		/// Combination of SceneObjectFlag::Enum
		unsigned GetObjFlag() const;
		/// Combination of SceneObjectFlag::Enum
		void ModifyObjFlag(unsigned flag, bool enable);
		/// Combination of SceneObjectFlag::Enum
		bool HasObjFlag(unsigned flag);
		void SetVisible(bool visible);
		bool GetVisible() const;

		//-------------------------------------------------------------------
		// Rendering
		//-------------------------------------------------------------------
		virtual void PreRender(const RenderParam& param, RenderParamOut* paramOut) = 0;
		virtual void Render(const RenderParam& param, RenderParamOut* paramOut) = 0;
		virtual void PostRender(const RenderParam& param, RenderParamOut* paramOut) = 0;

		//-------------------------------------------------------------------
		// Debugging features
		//-------------------------------------------------------------------
		void SetGameType(int type);
		int GetGameType() const;
		void SetGameId(unsigned id);
		unsigned GetGameId() const;
		void SetGamePtr(void* ptr);
		void* GetGamePtr() const;
	};
}