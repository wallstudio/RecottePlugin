#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fileapi.h>
#include <format>
#include <map>
#include <filesystem>
#include <memory>
#include <string>
#include "../HookHelper/HookHelper.h"

struct FilesProvider
{
    std::wstring fileName;
    std::vector<WIN32_FIND_DATAW> container;
    int current;

    FilesProvider(LPCWSTR lpFileName)
    {
        fileName = std::wstring(lpFileName);
        container = std::vector<WIN32_FIND_DATAW>();
        current = 0;
    }
};

std::map<HANDLE, std::shared_ptr<FilesProvider>> g_FileFindHandles = std::map<HANDLE, std::shared_ptr<FilesProvider>>();


BOOL _FindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData);
HANDLE _FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
    auto base = RecottePluginFoundation::LookupFunction<decltype(&_FindFirstFileW)>("kernel32.dll", "FindFirstFileW");
    auto result = base(lpFileName, lpFindFileData);

    {
         auto find = RecottePluginFoundation::LookupFunction<decltype(&_FindFirstFileW)>("kernel32.dll", "FindFirstFileW");
         auto next = RecottePluginFoundation::LookupFunction<decltype(&_FindNextFileW)>("kernel32.dll", "FindNextFileW");
         WIN32_FIND_DATAW data;
        g_FileFindHandles[result] = std::shared_ptr<FilesProvider>(new FilesProvider(lpFileName));
        for (auto handle = find(lpFileName, &data); handle != INVALID_HANDLE_VALUE && next(handle, &data);)
        {
             g_FileFindHandles[result]->container.push_back(data);
        }
    }

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
    auto provider = g_FileFindHandles[hFindFile];
    auto isValid = provider->current < provider->container.size();
    if (isValid)
    {
        auto data = &provider->container[provider->current];
        memcpy(lpFindFileData, data, sizeof(WIN32_FIND_DATAW));
        provider->current++;
    }

	OutputDebugStringW(std::format(L"[FileSystemBypass] _FindNextFileW {} {}\n", provider->fileName.c_str(), lpFindFileData->cFileName).c_str());
    return isValid;
}

BOOL _FindClose(HANDLE hFindFile)
{
    OutputDebugStringW(std::format(L"[FileSystemBypass] _FindClose {}\n", g_FileFindHandles[hFindFile]->fileName).c_str());
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
