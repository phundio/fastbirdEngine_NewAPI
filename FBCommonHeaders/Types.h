// Should not define data in this file.
// Only typedefs or defines are allowed.
#pragma once
#define FBCommonHeaders_Types_h
#include <utility>
#include <memory>

#define FB_DECLARE_HANDLE(name) struct name##__; typedef struct name##__ *name


namespace fastbird{
	typedef double Real;
	typedef unsigned char BYTE;
	typedef unsigned char UINT8;
	typedef unsigned int UINT;
	typedef unsigned long DWORD;
	typedef unsigned __int64 UINT64;
	typedef __int64 INT64;
	typedef __int64 HWindowId;
	FB_DECLARE_HANDLE(HWindow);

	typedef std::pair<int, int> CoordinatesI;
	typedef std::pair<Real, Real> CoordinatesR;
}

#define DECLARE_NON_COPYABLE(className) \
	className(const className&) = delete;\
	className& className::operator= (const className&) = delete

#define DECLARE_PIMPL(className) \
	class Impl; \
	std::shared_ptr<Impl> mImpl

#define DECLARE_PIMPL_NON_COPYABLE(className) \
	DECLARE_PIMPL(className); \
	DECLARE_NON_COPYABLE(className)

#define DECLARE_SMART_PTR(className) \
	class className;\
	typedef std::shared_ptr<className> className##Ptr;\
	typedef std::weak_ptr<className> className##WeakPtr

#define DECLARE_SMART_PTR_STRUCT(className) \
	struct className;\
	typedef std::shared_ptr<className> className##Ptr;\
	typedef std::weak_ptr<className> className##WeakPtr

#define IMPLEMENT_STATIC_CREATE(className)\
	className##Ptr className##::Create(){\
		return className##Ptr(new className, [](className* obj){delete obj;});\
	}