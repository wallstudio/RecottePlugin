#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fileapi.h>
#include <format>
#include <map>
#include <filesystem>
#include "../HookHelper/HookHelper.h"


std::map<HANDLE, std::filesystem::path> g_FileFindHandles = std::map<HANDLE, std::filesystem::path>();

HANDLE _FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
    auto base = RecottePluginFoundation::LookupFunction<decltype(&_FindFirstFileW)>("kernel32.dll", "FindFirstFileW");
    auto result = base(lpFileName, lpFindFileData);
    g_FileFindHandles[result] = std::filesystem::path(lpFileName);
	OutputDebugStringW(std::format(L"[FileSystemBypass] _FindFirstFileW {} {}\n", lpFileName, lpFindFileData->cFileName).c_str());
    return result;
}

HANDLE _FindFirstFileExW(LPCWSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags)
{
    auto base = RecottePluginFoundation::LookupFunction<decltype(&_FindFirstFileExW)>("kernel32.dll", "FindFirstFileExW");
    auto result = base(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
	OutputDebugStringW(std::format(L"[FileSystemBypass] _FindFirstFileExW {} {}\n", lpFileName, ((WIN32_FIND_DATA*)lpFindFileData)->cFileName).c_str());
    return result;
}

BOOL _FindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
    auto base = RecottePluginFoundation::LookupFunction<decltype(&_FindNextFileW)>("kernel32.dll", "FindNextFileW");
    auto result = base(hFindFile, lpFindFileData);
	OutputDebugStringW(std::format(L"[FileSystemBypass] _FindNextFileW {} {}\n", g_FileFindHandles[hFindFile].c_str(), lpFindFileData->cFileName).c_str());
    return result;
}

BOOL _FindClose(HANDLE hFindFile)
{
    OutputDebugStringW(std::format(L"[FileSystemBypass] _FindClose {}\n", g_FileFindHandles[hFindFile].c_str()).c_str());
    auto base = RecottePluginFoundation::LookupFunction<decltype(&_FindClose)>("kernel32.dll", "FindClose");
    g_FileFindHandles.erase(hFindFile);
    return base(hFindFile);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	OutputDebugStringW(L"[FileSystemBypass] " __FUNCTION__ "\n");
    RecottePluginFoundation::OverrideImportFunction("kernel32.dll", "FindFirstFileW", _FindFirstFileW);
    RecottePluginFoundation::OverrideImportFunction("kernel32.dll", "FindFirstFileExW", _FindFirstFileExW);
    RecottePluginFoundation::OverrideImportFunction("kernel32.dll", "FindNextFileW", _FindNextFileW);
    RecottePluginFoundation::OverrideImportFunction("kernel32.dll", "FindClose", _FindClose);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	OutputDebugStringW(L"[FileSystemBypass]" __FUNCTION__ "\n");
}
