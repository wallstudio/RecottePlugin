#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <map>
#include <fmt/format.h>
#include <math.h>
#include "../HookHelper/HookHelper.h"


HWND timeline = nullptr;
HWND timelineLabelList = nullptr;
std::map<HWND, WNDPROC> g_OriginalProc = std::map<HWND, WNDPROC>();


HWND CreateWindowExW(
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

	typedef HWND(WINAPI* Callback)(
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
	auto base = RecottePluginFoundation::LookupFunction<Callback>("user32.dll", "CreateWindowExW");
	if (base == nullptr)
	{
		return nullptr;
	}

	HWND hwnd = nullptr;
	if (lpWindowName != nullptr && std::wstring(lpWindowName) == L"タイムライン")
	{
		hwnd = base(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		timeline = hwnd;
	}
	else if (timeline != nullptr && hWndParent == timeline)
	{
		hwnd = base(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

		auto index = 0;
		HWND prev = hwnd;
		while (nullptr != (prev = GetNextWindow(prev, GW_HWNDPREV))) index++;
		switch (index)
		{
		case 0:
			SetWindowTextW(hwnd, L"Timeline_LabelHeader");
			break;
		case 1:
			SetWindowTextW(hwnd, L"Timeline_LabelList");
			timelineLabelList = hwnd;
			break;
		case 2:
			SetWindowTextW(hwnd, L"Timeline_Main");
			break;
		case 3:
			SetWindowTextW(hwnd, L"Timeline_VerticalScrollbar");
			break;
		case 4:
			SetWindowTextW(hwnd, L"Timeline_HorizontalScrollbar");
			break;
		case 5:
			SetWindowTextW(hwnd, L"Timeline_Unknown");
			break;
		case 6:
			SetWindowTextW(hwnd, L"Timeline_Toolbar");
			break;
		}
	}
	else if (timelineLabelList != nullptr && hWndParent == timelineLabelList)
	{
		hwnd = base(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, 10, hWndParent, hMenu, hInstance, lpParam);
		SetWindowTextW(hwnd, L"TimelineLabelItem");

		WNDPROC hookProce = [](HWND h, UINT u, WPARAM w, LPARAM l) -> LRESULT
		{
			if (u == WM_WINDOWPOSCHANGING)
			{
				auto params = reinterpret_cast<WINDOWPOS*>(l);
				OutputDebugStringW(fmt::format(L"[TestPlugin] WM_SIZE ({0}x{1})", params->cx, params->cy).c_str());
				params->cy = 10;
			}
			return g_OriginalProc[h](h, u, w, l);
		};
		g_OriginalProc[hwnd] = g_OriginalProc.count(hwnd) > 0 ? g_OriginalProc[hwnd] : reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
		SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hookProce));
	}
	else
	{
		hwnd = base(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	}

	return hwnd;
}

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	OutputDebugStringW(L"[TestPlugin] OnPluginStart\n");
	RecottePluginFoundation::OverrideImportFunction("user32.dll", "CreateWindowExW", CreateWindowExW);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	OutputDebugStringW(L"[TestPlugin] OnPluginFinish\n");
}
