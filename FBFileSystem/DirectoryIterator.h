#pragma once
#include <memory>
#include "FBCommonHeaders/String.h"
#include "FBCommonHeaders/Types.h"
namespace fastbird{
	class DirectoryIterator;
	typedef std::shared_ptr<DirectoryIterator> DirectoryIteratorPtr;
	typedef std::weak_ptr<DirectoryIterator> DirectoryIteratorWeakPtr;

	/** Read file in a directory.
	Every time you call \b GetNextFileName() you will get a file name in the
	directory. Order is not defined. You will get empty string _T("") if you get
	all file names. This function is not thread safe.
	*/
	class FB_DLL_PUBLIC DirectoryIterator{
		DECLARE_PIMPL_NON_COPYABLE(DirectoryIterator);

	public:
		DirectoryIterator(const char* directoryPath, bool recursive);
		~DirectoryIterator();

		bool HasNext() const;
		/** Get next file name
		Order is undefined.
		\return A file name or the empty string _T("") when every file names
		returned.
		*/
		const char* GetNextFileName(bool* outIsDirectory);
		const char* GetNextFileName();
	};
}