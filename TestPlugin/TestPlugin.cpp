#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <set>
#include <fmt/format.h>
#include <math.h>
#include "../HookHelper/HookHelper.h"


std::set<HWND> g_TimelineItems = std::set<HWND>();


typedef HWND(*CreateWindowExWCallback)(
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
	auto hwnd = base(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

	if (hWndParent != nullptr)
	{
		static HWND timelineLabelList = nullptr;
		if (timelineLabelList == nullptr)
		{
			auto buff = std::vector<wchar_t>(256);
			GetWindowTextW(hWndParent, buff.data(), buff.size());
			auto parentText = std::wstring(buff.data());
			if (parentText == L"タイムライン")
			{
				auto header = GetNextWindow(hwnd, GW_HWNDPREV);
				if (header != nullptr && GetNextWindow(header, GW_HWNDPREV) == nullptr)
				{
					SetWindowTextW(hwnd, L"TimelineLabelList");
					timelineLabelList = hwnd;
				}
			}
		}

		if (hWndParent == timelineLabelList)
		{
			SetWindowTextW(hwnd, L"TimelineLabelItem");
			g_TimelineItems.insert(hwnd);
			static WNDPROC defaultProc = nullptr;
			if (defaultProc == nullptr)
			{
				defaultProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
			}
			WNDPROC HookProce = [](HWND h, UINT u, WPARAM w, LPARAM l) -> LRESULT
			{
				if(u == WM_SIZE)
				{
					auto width = LOWORD(l);
					auto height = HIWORD(l);
					OutputDebugStringW(fmt::format(L"[TestPlugin] WM_SIZE ({0},{1})", width, height).c_str());
					height = min(height, 10);
					l = MAKELONG(width, height);

					RECT r;
					GetWindowRect(h, &r);
					MoveWindow(h, 0, 0, width, height, true);
					// なんかうまくいかない…消える
					return 0;
				}
				return defaultProc(h, u, w, l); // こっちに回すと変わらない
			};
			SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookProce));
		}
	}

	return hwnd;
}


extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	OutputDebugStringW(L"[TestPlugin] OnPluginStart\n");
	RecottePluginFoundation::OverrideImportFunction("user32.dll", "CreateWindowExW", HookCreateWindowExW);
	//RecottePluginFoundation::OverrideImportFunction("user32.dll", "MoveWindow", HookMoveWindow);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	OutputDebugStringW(L"[TestPlugin] OnPluginFinish\n");
}
