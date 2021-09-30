#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fileapi.h>
#include <format>
#include <map>
#include <filesystem>
#include <memory>
#include <string>
#include "../HookHelper/HookHelper.h"


decltype(&FindFirstFileW) g_Original_FindFirstFileW;
decltype(&FindNextFileW) g_Original_FindNextFileW;
decltype(&FindClose) g_Original_FindClose;
decltype(&CreateFileW) g_Original_CreateFileW;


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

std::filesystem::path ResolveRecotteShaderDirctory()
{
	std::vector<wchar_t> buffer;
	size_t buffSize;

	// 環境変数モード（for Dev）
	_wgetenv_s(&buffSize, nullptr, 0, L"RECOTTE_SHADER_DIR");
	if (buffSize != 0)
	{
		buffer = std::vector<wchar_t>(buffSize);
		_wgetenv_s(&buffSize, buffer.data(), buffer.size(), L"RECOTTE_SHADER_DIR");
		return std::filesystem::path(buffer.data());
	}

	static auto pluginDir = RecottePluginManager::ResolvePluginPath();
	return pluginDir / "RecotteShader";
}


HANDLE _FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
	auto result = g_Original_FindFirstFileW(lpFileName, lpFindFileData);

	// Plugin Effectを差し込む
	static LPCWSTR types[]{ L"effects", L"text", L"transitions" };
	for (auto type : types)
	{
		if (lpFileName == std::format(L"C:\\Program Files\\RecotteStudio\\effects\\{}\\*", type))
		{
			WIN32_FIND_DATAW data;
			g_FileFindHandles[result] = std::shared_ptr<FilesProvider>(new FilesProvider(lpFileName));
			for (auto handle = g_Original_FindFirstFileW(lpFileName, &data); handle != INVALID_HANDLE_VALUE && g_Original_FindNextFileW(handle, &data);)
			{
				g_FileFindHandles[result]->container.push_back(data);
			}

			static auto recotteShaderDir = ResolveRecotteShaderDirctory();
			auto dir = recotteShaderDir / type;
			dir = dir.lexically_relative(std::format(L"C:\\Program Files\\RecotteStudio\\effects\\{}", type));
			for (auto handle = g_Original_FindFirstFileW(std::format(L"{}\\*", dir.wstring()).c_str(), &data); handle != INVALID_HANDLE_VALUE && g_Original_FindNextFileW(handle, &data);)
			{
				if (0 == wcscmp(data.cFileName, L"..")) continue;
				auto relative = dir / data.cFileName;
				wcscpy_s(data.cFileName, MAX_PATH, relative.c_str());
				g_FileFindHandles[result]->container.push_back(data);
			}
		}
	}
	return result;
}

BOOL _FindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
	if (!g_FileFindHandles.contains(hFindFile))
	{
		return g_Original_FindNextFileW(hFindFile, lpFindFileData);
	}

	auto provider = g_FileFindHandles[hFindFile];
	auto isValid = provider->current < provider->container.size();
	if (isValid)
	{
		auto data = &provider->container[provider->current];
		memcpy(lpFindFileData, data, sizeof(WIN32_FIND_DATAW));
		provider->current++;
	}
	return isValid;
}

BOOL _FindClose(HANDLE hFindFile)
{
	if (g_FileFindHandles.contains(hFindFile))
	{
		g_FileFindHandles.erase(hFindFile);
	}

	return g_Original_FindClose(hFindFile);
}

HANDLE _CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	auto path = std::filesystem::path(lpFileName);
	if (path.parent_path() == L".\\recotte_shader_effect_lib")
	{
		static auto recotteShaderDir = ResolveRecotteShaderDirctory();
		path = recotteShaderDir / "recotte_shader_effect_lib" / path.filename();
		lpFileName = path.c_str();
	}
	return g_Original_CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	g_Original_FindFirstFileW = RecottePluginManager::OverrideIATFunction("kernel32.dll", "FindFirstFileW", _FindFirstFileW);
	g_Original_FindNextFileW = RecottePluginManager::OverrideIATFunction("kernel32.dll", "FindNextFileW", _FindNextFileW);
	g_Original_FindClose = RecottePluginManager::OverrideIATFunction("kernel32.dll", "FindClose", _FindClose);
	g_Original_CreateFileW = RecottePluginManager::OverrideIATFunction("kernel32.dll", "CreateFileW", _CreateFileW);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld) {}
