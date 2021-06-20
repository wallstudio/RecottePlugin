#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <fmt/format.h>
#include <math.h>
#include "../HookHelper/HookHelper.h"
#include "Gdip.h"


HWND timeline = nullptr;
HWND timelineLabelList = nullptr;
HWND timelineLayers = nullptr;
struct TimelneLabel
{
	bool Folding;
	WNDPROC OriginalProc, BoxOriginalProc;
	SIZE ActualSize;
};
std::map<HWND, TimelneLabel*> g_TimelLineLabelFoldings = std::map<HWND, TimelneLabel*>();


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

	auto base = RecottePluginFoundation::LookupFunction<decltype(&CreateWindowExW)>("user32.dll", "CreateWindowExW");
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
			SetWindowTextW(hwnd, L"Timeline_Layers");
			timelineLayers = hwnd;
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
			SetWindowTextW(hwnd, L"Timeline_Layers");
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
		hwnd = base(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		SetWindowTextW(hwnd, L"TimelineLabelItem");
		g_TimelLineLabelFoldings[hwnd] = new TimelneLabel{ .ActualSize = {nWidth, nHeight} };

		WNDPROC proc = [](HWND h, UINT u, WPARAM w, LPARAM l) -> LRESULT
		{
			if (u == WM_WINDOWPOSCHANGING)
			{
				auto params = reinterpret_cast<WINDOWPOS*>(l);
				g_TimelLineLabelFoldings[h]->ActualSize = { params->cx, params->cy };
				if (!g_TimelLineLabelFoldings[h]->Folding)
				{
					params->cy = 67;
				}
			}
			return g_TimelLineLabelFoldings[h]->OriginalProc(h, u, w, l);
		};
		g_TimelLineLabelFoldings[hwnd]->OriginalProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
		SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(proc));
	}
	else if (timelineLayers != nullptr && hWndParent == timelineLayers)
	{
		// Layer側はWindowではなくHDCで描かれている…
		// データ探してきて再計算・再描画して上に描くしかない？

		//hwnd = base(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		//SetWindowTextW(hwnd, L"TimelineLayersItem");
		//auto index = 0;
		//HWND prev = hwnd;
		//while (nullptr != (prev = GetNextWindow(prev, GW_HWNDPREV))) index++;
		//auto label =  timelineLabelList
		//for (size_t i = 0; i < index; i++)
		//{

		//}
		//g_TimelLineLabelFoldings[hwnd] = new TimelneLabel{ .ActualSize = {nWidth, nHeight} };

		//WNDPROC proc = [](HWND h, UINT u, WPARAM w, LPARAM l) -> LRESULT
		//{
		//	if (u == WM_WINDOWPOSCHANGING)
		//	{
		//		auto params = reinterpret_cast<WINDOWPOS*>(l);
		//		g_TimelLineLabelFoldings[h]->ActualSize = { params->cx, params->cy };
		//		if (!g_TimelLineLabelFoldings[h]->Folding)
		//		{
		//			params->cy = 67;
		//		}
		//	}
		//	return g_TimelLineLabelFoldings[h]->OriginalProc(h, u, w, l);
		//};
		//g_TimelLineLabelFoldings[hwnd]->OriginalProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
		//SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(proc));
	}
	else if (g_TimelLineLabelFoldings.contains(hWndParent))
	{
		hwnd = base(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		
		auto index = 0;
		HWND prev = hwnd;
		while (nullptr != (prev = GetNextWindow(prev, GW_HWNDPREV))) index++;
		switch (index)
		{
		case 0:
			SetWindowTextW(hwnd, L"Timeline_Label_Icon");
			break;
		case 1:
			SetWindowTextW(hwnd, L"Timeline_Label_Box");
			{
				WNDPROC proc = [](HWND h, UINT u, WPARAM w, LPARAM l) -> LRESULT
				{
					auto label = GetParent(h);
					if (u == WM_LBUTTONUP)
					{
						auto labelData = g_TimelLineLabelFoldings[label];
						labelData->Folding = !labelData->Folding;
						SetWindowPos(label, HWND_TOP, 0, 0, labelData->ActualSize.cx, labelData->ActualSize.cy, SWP_NOMOVE| SWP_NOOWNERZORDER);
					}
					return g_TimelLineLabelFoldings[label]->BoxOriginalProc(h, u, w, l);
				};
				g_TimelLineLabelFoldings[hWndParent]->BoxOriginalProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
				SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(proc));
			}
			break;
		case 2:
			SetWindowTextW(hwnd, L"Timeline_Label_Text");
			break;
		}
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
	//RecottePluginFoundation::OverrideImportFunction("user32.dll", "CreateWindowExW", CreateWindowExW);

	//for (auto funcInfo : GetOverrideFunctions())
	//{
	//	RecottePluginFoundation::OverrideImportFunction(funcInfo.Module, funcInfo.FuncName, funcInfo.Func);
	//}
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	OutputDebugStringW(L"[TestPlugin] OnPluginFinish\n");
}
