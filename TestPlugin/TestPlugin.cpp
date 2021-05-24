#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <thread>
#include <vector>
#include <string>
#include <codecvt>
#include <map>
#include <fmt/format.h>
#include "../HookHelper/HookHelper.h"


typedef HWND (*CreateWindowExWCallback)(
	_In_ DWORD dwExStyle,
	_In_opt_ LPCWSTR lpClassName,
	_In_opt_ LPCWSTR lpWindowName,
	_In_ DWORD dwStyle,
	_In_ int X,
	_In_ int Y,
	_In_ int nWidth,
	_In_ int nHeight,
	_In_opt_ HWND hWndParent,
	_In_opt_ HMENU hMenu,
	_In_opt_ HINSTANCE hInstance,
	_In_opt_ LPVOID lpParam);

HWND HookCreateWindowExW(
	_In_ DWORD dwExStyle,
	_In_opt_ LPCWSTR lpClassName,
	_In_opt_ LPCWSTR lpWindowName,
	_In_ DWORD dwStyle,
	_In_ int X,
	_In_ int Y,
	_In_ int nWidth,
	_In_ int nHeight,
	_In_opt_ HWND hWndParent,
	_In_opt_ HMENU hMenu,
	_In_opt_ HINSTANCE hInstance,
	_In_opt_ LPVOID lpParam)
{
	OutputDebugStringW(fmt::format(L"[TestPlugin] CreateWindowExW: {0} / {1}\n", lpClassName != nullptr ? lpClassName : L"", lpWindowName != nullptr ? lpWindowName : L"").c_str());
	auto base = RecottePluginFoundation::LookupFunction<CreateWindowExWCallback>("user32.dll", "CreateWindowExW");
	if (base == nullptr)
	{
		return nullptr;
	}
	return base(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	OutputDebugStringW(L"[TestPlugin] OnPluginStart");
	RecottePluginFoundation::OverrideImportFunction("user32.dll", "CreateWindowExW", HookCreateWindowExW);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	OutputDebugStringW(L"[TestPlugin] OnPluginFinish");
}
