/**
 * Appcelerator Kroll - licensed under the Apache Public License 2
 * see LICENSE in the root folder for details on the license.
 * Copyright (c) 2008 Appcelerator, Inc. All Rights Reserved.
 */

#include <windows.h>
#include <shlobj.h>
#include <Iphlpapi.h>
#include <process.h>
#include <shellapi.h>
#include <sstream>


#include "../file_utils.h"
#include "../win32/win32_utils.h"

namespace UTILS_NS
{
namespace FileUtils
{
	static bool FileHasAttributes(const std::wstring& widePath, DWORD attributes)
	{
		WIN32_FIND_DATA findFileData;
		ZeroMemory(&findFileData, sizeof(WIN32_FIND_DATA));

		HANDLE hFind = FindFirstFile(widePath.c_str(), &findFileData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			FindClose(hFind);

			// We do this check, because we may have passed in 0 for attributse
			// in which case the callee just wants to know if the path exists.
			return (findFileData.dwFileAttributes & attributes) == attributes;
		}
		return false;
	}

	static bool FileHasAttributes(const std::string& path, DWORD attributes)
	{
		std::wstring widePath(UTILS_NS::UTF8ToWide(path));
		return FileHasAttributes(widePath, attributes);
	}

	std::string GetExecutableDirectory()
	{
		wchar_t path[MAX_PATH];
		path[MAX_PATH-1] = '\0';
		int size = GetModuleFileNameW(NULL, path, MAX_PATH - 1);
		if (size > 0)
		{
			std::string fullPath(UTILS_NS::WideToUTF8(path));
			return Dirname(fullPath);
		}
		return std::string("");
	}


	std::string GetTempDirectory()
	{
		wchar_t tempDirectory[MAX_PATH];
		tempDirectory[MAX_PATH-1] = '\0';
		GetTempPathW(MAX_PATH - 1, tempDirectory);
		
		// This function seem to return Windows stubby paths, so
		// let's convert it to a full path name.
		std::wstring dir(tempDirectory);
		GetLongPathNameW(dir.c_str(), tempDirectory, MAX_PATH - 1);
		std::string out(UTILS_NS::WideToUTF8(tempDirectory));
		srand(GetTickCount()); // initialize seed
		std::ostringstream s;
		s << "k" << (double) rand();
		std::string end(s.str());
		return FileUtils::Join(out.c_str(), end.c_str(), NULL);
	}

	bool IsFile(const std::string& file)
	{
		return FileHasAttributes(file, 0);
	}

	bool IsFile(const std::wstring& file)
	{
		return FileHasAttributes(file, 0);
	}

	void WriteFile(const std::string& path, const std::string& content)
	{
		std::wstring widePath(UTF8ToWide(path));
		// CreateFile doesn't have a path length limitation
		HANDLE file = CreateFileW(widePath.c_str(),
			GENERIC_WRITE, 
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		
		DWORD bytesWritten = 0;
		DWORD bytesToWrite = (DWORD)content.size();
		
		if (file != INVALID_HANDLE_VALUE)
		{
			while (bytesWritten < bytesToWrite)
			{
				if (FALSE == ::WriteFile(file, content.c_str() + bytesWritten,
					bytesToWrite - bytesWritten, &bytesWritten, NULL))
				{
					CloseHandle(file);
					fprintf(stderr, "Could not write to file. Error: %s\n",
						Win32Utils::QuickFormatMessage(GetLastError()).c_str());
				}
			}
		}

		CloseHandle(file);
	}
	
	std::string ReadFile(const std::string& path)
	{
		return ReadFile(UTF8ToWide(path));
	}
	
	std::string ReadFile(const std::wstring& widePath)
	{
		std::ostringstream contents;
		// CreateFile doesn't have a path length limitation
		HANDLE file = CreateFileW(widePath.c_str(), GENERIC_READ, 
			FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
			NULL);
		
		DWORD bytesRead;
		const int bufferSize = 4096;
		char readBuffer[bufferSize];
		
		do
		{
			BOOL result = ::ReadFile(file, readBuffer,
				bufferSize-2, &bytesRead, NULL);
			
			if (!result && GetLastError() != ERROR_HANDLE_EOF)
			{
				fprintf(stderr, "Could not read from file(%s). Error: %s\n",
					UTILS_NS::WideToUTF8(widePath).c_str(),
					Win32Utils::QuickFormatMessage(GetLastError()).c_str());
				CloseHandle(file);
				return std::string();
			}
			readBuffer[bytesRead] = '\0'; // NULL character
			contents << readBuffer;
		} while (bytesRead > 0);
		
		CloseHandle(file);
		return contents.str();
	}
	
	std::string Dirname(const std::string& path)
	{
		wchar_t pathBuffer[_MAX_PATH];
		wchar_t drive[_MAX_DRIVE];
		wchar_t dir[_MAX_DIR];
		wchar_t fname[_MAX_FNAME];
		wchar_t ext[_MAX_EXT];

		std::wstring widePath(UTILS_NS::UTF8ToWide(path));
		wcsncpy(pathBuffer, widePath.c_str(), MAX_PATH - 1);
		pathBuffer[MAX_PATH - 1] = '\0';

		_wsplitpath(pathBuffer, drive, dir, fname, ext);
		if (dir[wcslen(dir)-1] == '\\')
			dir[wcslen(dir)-1] = '\0';

		std::wstring dirname(drive);
		dirname.append(dir);
		return UTILS_NS::WideToUTF8(dirname);
	}
	
	bool CreateDirectoryImpl(const std::string& dir)
	{
		std::wstring wideDir(UTILS_NS::UTF8ToWide(dir));
		return (::CreateDirectoryW(wideDir.c_str(), NULL) == TRUE);
	}

	bool DeleteFile(const std::string& path)
	{
		// SHFileOperation doesn't care if it's a dir or file -- delegate
		// Use DeleteFile here instead of SHFileOperation (DeleteDirectory),
		// because that is the best way to ensure that the file does not
		// end up in the Recycle Bin and DeleteFile has saner handling
		// of paths with spaces.
		//return ::DeleteFile(path);
		return DeleteDirectory(path);
	}
	
	bool DeleteDirectory(const std::string& dir)
	{
		std::wstring wideDir(UTILS_NS::UTF8ToWide(dir));

		// SHFileOperation does not handle paths with spaces well, so
		// convert to a Windows stubby path here. Ugh.
		wchar_t stubbyPath[MAX_PATH];
		GetShortPathNameW(wideDir.c_str(), stubbyPath, MAX_PATH - 1);
		
		SHFILEOPSTRUCT op;
		op.hwnd = NULL;
		op.wFunc = FO_DELETE;
		op.pFrom = stubbyPath;
		op.pTo = NULL;
		op.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
		int rc = SHFileOperation(&op);
		return (rc == 0);
	}

	bool IsDirectory(const std::string& path)
	{
		return FileHasAttributes(path, FILE_ATTRIBUTE_DIRECTORY);
	}

	std::string GetUserRuntimeHomeDirectory()
	{
		wchar_t widePath[MAX_PATH];
		if (SHGetSpecialFolderPath(NULL, widePath, CSIDL_APPDATA, FALSE))
		{
			std::string path(UTILS_NS::WideToUTF8(widePath));
			return Join(path.c_str(), PRODUCT_NAME, NULL);
		}
		else
		{
			// Not good! What do we do in this case? I guess just use  a reasonable 
			// default for Windows. Ideally this should *never* happen.
			return Join("C:", PRODUCT_NAME, NULL);
		}
	}
	
	std::string GetSystemRuntimeHomeDirectory()
	{
		wchar_t widePath[MAX_PATH];
		if (SHGetSpecialFolderPath(NULL, widePath, CSIDL_COMMON_APPDATA, FALSE))
		{
			std::string path(UTILS_NS::WideToUTF8(widePath));
			return Join(path.c_str(), PRODUCT_NAME, NULL);
		}
		else
		{
			return GetUserRuntimeHomeDirectory();
		}
	}

	bool IsHidden(const std::string& path)
	{
		return FileHasAttributes(path, FILE_ATTRIBUTE_HIDDEN);
	}

	void ListDir(const std::string& path, std::vector<std::string> &files)
	{
		if (!IsDirectory(path))
			return;

		files.clear();

		WIN32_FIND_DATA findFileData;
		ZeroMemory(&findFileData, sizeof(WIN32_FIND_DATA));

		std::wstring widePath(UTILS_NS::UTF8ToWide(path));
		std::wstring searchString(widePath + L"\\*");

		HANDLE hFind = FindFirstFile(searchString.c_str(), &findFileData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				std::wstring wideFilename(findFileData.cFileName);
				if (wideFilename != L"." && wideFilename != L"..")
				{
					std::string filename(UTILS_NS::WideToUTF8(wideFilename));
					files.push_back(filename);
				}

			} while (FindNextFile(hFind, &findFileData));
			FindClose(hFind);
		}
	}

	int RunAndWait(const std::string& path, std::vector<std::string> &args)
	{
		std::string cmdLine = "\"" + path + "\"";
		for (size_t i = 0; i < args.size(); i++)
		{
			cmdLine += " \"" + args.at(i) + "\"";
		}

#ifdef DEBUG
		std::cout << "running: " << cmdLine << std::endl;
#endif

		STARTUPINFO startupInfo;
		ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
		PROCESS_INFORMATION processInfo;
		ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));
		startupInfo.cb = sizeof(STARTUPINFO);

		// Get the current working directory
		wchar_t cwd[MAX_PATH];
		DWORD size = GetCurrentDirectoryW(MAX_PATH, (wchar_t*) cwd);
		std::wstring wideCmdLine(UTILS_NS::UTF8ToWide(cmdLine));

		DWORD rc = -1;
		if (CreateProcessW(
			NULL,                           // No module name (use command line)
			(wchar_t*) wideCmdLine.c_str(), // Command line
			NULL,                           // Process handle not inheritable
			NULL,                           // Thread handle not inheritable
			FALSE,                          // Set handle inheritance to FALSE
			0,                              // No creation flags
			NULL,                           // Use parent's environment block
			(wchar_t*) cwd,                 // Use parent's starting directory
			&startupInfo,                   // Pointer to STARTUPINFO structure
			&processInfo))                  // Pointer to PROCESS_INFORMATION structure
		{
			// Wait until child process exits.
			WaitForSingleObject(processInfo.hProcess, INFINITE);

			// set the exit code
			GetExitCodeProcess(processInfo.hProcess, &rc);

			// Close process and thread handles.
			CloseHandle(processInfo.hProcess);
			CloseHandle(processInfo.hThread);
		}
		return rc;
	}

	// TODO: implement this for other platforms
	void CopyRecursive(const std::string& dir, const std::string& dest, const std::string& exclude)
	{ 
		if (!IsDirectory(dest)) 
		{
			CreateDirectory(dest);
		}

#ifdef DEBUG
std::cout << "\n>Recursive copy " << dir << " to " << dest << std::endl;
#endif
		std::vector<std::string> files;
		ListDir(dir, files);
		for (size_t i = 0; i < files.size(); i++)
		{
			std::string& filename = files[i];
			if (!exclude.empty() && exclude == filename)
				continue;

			std::string srcName(Join(dir.c_str(), filename.c_str(), NULL));
			std::string destName(Join(dest.c_str(), filename.c_str(), NULL));

			if (IsDirectory(srcName))
			{
#ifdef DEBUG
				std::cout << "create dir: " << destName << std::endl;
#endif
				CreateDirectory(destName);
				CopyRecursive(srcName, destName);
			}
			else
			{
				std::wstring wideSrcName(UTILS_NS::UTF8ToWide(srcName));
				std::wstring wideDestName(UTILS_NS::UTF8ToWide(destName));
				CopyFileW(wideSrcName.c_str(), wideDestName.c_str(), FALSE);
			}
		}
	}
}
}
