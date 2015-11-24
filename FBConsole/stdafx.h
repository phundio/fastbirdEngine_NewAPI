#include "FBCommonHeaders/platform.h"
#if defined(_PLATFORM_WINDOWS_)
#define FB_DLL_CONSOLE __declspec(dllexport)
#define FB_DLL_RENDERER __declspec(dllimport)
#define FB_DLL_LUA __declspec(dllimport)
#define FB_DLL_TIMER __declspec(dllimport)
#define FB_DLL_INPUTMANAGER __declspec(dllimport)
#else
#define FB_DLL_ENGINEFACADE 
#endif

#include "FBRenderer/Renderer.h"
#include "FBLua/LuaUtils.h"
#include "FBMemoryManagerLib/MemoryManager.h"
#include "FBTimer/Timer.h"
#include "FBDebugLib/DebugLib.h"

#include <vector>