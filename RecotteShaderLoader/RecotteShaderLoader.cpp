#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fileapi.h>
#include <format>
#include <list>
#include <map>
#include <filesystem>
#include <memory>
#include <string>
#include "../HookHelper/HookHelper.h"


auto g_ShaderFinding = std::map<HANDLE, std::shared_ptr<std::list<WIN32_FIND_DATAW>>>();


std::filesystem::path ResolveRecotteShaderDirctory()
{
	static auto shaderDirectory = std::filesystem::path();
	if (shaderDirectory.empty())
	{
		shaderDirectory = RecottePluginManager::ResolvePluginPath() / "RecotteShader";

		std::vector<wchar_t> buffer;
		size_t buffSize;
		_wgetenv_s(&buffSize, nullptr, 0, L"RECOTTE_SHADER_DIR");
		if (buffSize != 0)
		{
			// 環境変数モード（for Dev）
			buffer = std::vector<wchar_t>(buffSize);
			_wgetenv_s(&buffSize, buffer.data(), buffer.size(), L"RECOTTE_SHADER_DIR");
			shaderDirectory = buffer.data();
		}
	}
	return shaderDirectory;
}

HANDLE _FindFirstFileW(decltype(&FindFirstFileW) base, LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
	auto result = base(lpFileName, lpFindFileData);

	// Plugin Effectを差し込む
	static LPCWSTR types[]{ L"effects", L"text", L"transitions" };
	for (auto type : types)
	{
		if (lpFileName == RecottePluginManager::ResolveApplicationDir() / L"effects" / type / "*")
		{
			// 通常の子ファイルを列挙
			WIN32_FIND_DATAW data;
			auto directory = std::shared_ptr<std::list<WIN32_FIND_DATAW>>(new std::list<WIN32_FIND_DATAW>());
			for (auto handle = base(lpFileName, &data); handle != INVALID_HANDLE_VALUE && FindNextFileW(handle, &data);)
			{
				directory->push_back(data);
			}

			// RecotteShaderPluginディレクトリ内のファイルをリダイレクト
			auto dir = ResolveRecotteShaderDirctory() / type;
			dir = dir.lexically_relative(RecottePluginManager::ResolveApplicationDir() / L"effects" / type);
			for (auto handle = base((dir / L"*").c_str(), &data); handle != INVALID_HANDLE_VALUE && FindNextFileW(handle, &data);)
			{
				if (0 == wcscmp(data.cFileName, L"..")) continue;
				auto relative = dir / data.cFileName;
				wcscpy_s(data.cFileName, MAX_PATH, relative.c_str());
				directory->push_back(data);
			}
			g_ShaderFinding[result] = directory; // 先にmap入れるとFindNextFileWの挙動が変わるので遅延
		}
	}
	return result;
}

BOOL _FindNextFileW(decltype(&FindNextFileW) base, HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
	if (!g_ShaderFinding.contains(hFindFile)) return base(hFindFile, lpFindFileData);
	if (g_ShaderFinding[hFindFile]->size() == 0) return false;

	memcpy(lpFindFileData, &g_ShaderFinding[hFindFile]->front(), sizeof(WIN32_FIND_DATAW));
	g_ShaderFinding[hFindFile]->pop_front();
	return true;
}

BOOL _FindClose(decltype(&FindClose) base, HANDLE hFindFile)
{
	g_ShaderFinding.erase(hFindFile);
	return base(hFindFile);
}

HANDLE _CreateFileW(decltype(&CreateFileW) base, LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	auto filePath = std::filesystem::path(lpFileName);
	// includeをリダイレクト
	if (filePath.parent_path() == L".\\recotte_shader_effect_lib")
	{
		filePath = (ResolveRecotteShaderDirctory() / "recotte_shader_effect_lib" / filePath.filename()).c_str();
	}
	return base(filePath.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	static decltype(&FindFirstFileW) findFirstFileW = RecottePluginManager::OverrideIATFunction<decltype(&FindFirstFileW)>("kernel32.dll", "FindFirstFileW", [](auto ...args) { return _FindFirstFileW(findFirstFileW, args...); });
	static decltype(&FindNextFileW) findNextFileW = RecottePluginManager::OverrideIATFunction<decltype(&FindNextFileW)>("kernel32.dll", "FindNextFileW", [](auto ...args) { return _FindNextFileW(findNextFileW, args...); });
	static decltype(&FindClose) findClose = RecottePluginManager::OverrideIATFunction<decltype(&FindClose)>("kernel32.dll", "FindClose", [](auto ...args) { return _FindClose(findClose, args...); });
	static decltype(&CreateFileW) createFileW = RecottePluginManager::OverrideIATFunction<decltype(&CreateFileW)>("kernel32.dll", "CreateFileW", [](auto ...args) { return _CreateFileW(createFileW, args...); });
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld) {}
