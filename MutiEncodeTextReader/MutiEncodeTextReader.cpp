#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fileapi.h>
#include <format>
#include <map>
#include <filesystem>
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <combaseapi.h>
#include <Mlang.h>
#include <wrl.h>
#include "../HookHelper/HookHelper.h"

#pragma comment(lib,"Ole32.lib")

void* g_BaseCreateFileW;
Microsoft::WRL::ComPtr<IMultiLanguage2> ml = nullptr;

HANDLE _CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    auto path = std::filesystem::path(lpFileName);
    if (path.extension() == ".txt")
    {
        auto alt = path.parent_path() / (path.filename().wstring() + L".sjis.txt");
        if (!std::filesystem::exists(alt))
        {
            HRESULT hr;
            if (ml == nullptr) hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&ml));
            _ASSERTE(_CrtCheckMemory());
            // Read
            DWORD farFileSize;
            long fileSize = std::filesystem::file_size(path);
            auto srcBuff = std::vector<char>(fileSize);
            auto sr = std::ifstream(path);
            sr.read(srcBuff.data(), fileSize);
            _ASSERTE(_CrtCheckMemory());
            // Detect
            int buffLen = srcBuff.size(), infoLen = 4;
            auto info = std::vector<DetectEncodingInfo>(infoLen);
            hr = ml->DetectInputCodepage(MLDETECTCP_NONE, 0, srcBuff.data(), &buffLen, info.data(), &infoLen);
            _ASSERTE(_CrtCheckMemory());
            // Convert
            DWORD mode = 0;
            auto dstBuff = std::vector<char>(fileSize * 4);
            UINT srcSize = srcBuff.size(), dstSize = dstBuff.size();
            hr = ml->ConvertString(&mode, info[0].nCodePage, 932, (BYTE*)srcBuff.data(), &srcSize, (BYTE*)dstBuff.data(), &srcSize);
            dstBuff.resize(dstSize);
            _ASSERTE(_CrtCheckMemory());
            // Write
            auto sw = std::ofstream(alt);
            sw.write(dstBuff.data(), dstBuff.size());
            _ASSERTE(_CrtCheckMemory());
        }
        lpFileName = alt.c_str();
    }

    auto result = ((decltype(&CreateFileW))g_BaseCreateFileW)(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    OutputDebugStringW(std::format(L"[MutiEncodeTextReader] _ReadFile {}\n", path.c_str()).c_str());
    return result;
}

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
    OutputDebugStringW(L"[RecotteShaderLoader] " __FUNCTION__ "\n");

    g_BaseCreateFileW = (void*)RecottePluginFoundation::FindImportAddress("kernel32.dll", "CreateFileW")->u1.AddressOfData;
    RecottePluginFoundation::OverrideImportFunction("kernel32.dll", "CreateFileW", _CreateFileW);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
    OutputDebugStringW(L"[RecotteShaderLoader]" __FUNCTION__ "\n");
}
