#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <format>
#include <math.h>
#include "../HookHelper/HookHelper.h"


HWND TimelineWindow = nullptr;
HWND TimelineWidnowLabels = nullptr;
HWND TimelineMainWidnow = nullptr;
struct TimelineLabelItemExSetting
{
	HWND Hwnd;
	bool Folding;
	WNDPROC BoxOriginalProc;
};
std::map<HWND, TimelineLabelItemExSetting*> TimelineWidnowLabelsItems = std::map<HWND, TimelineLabelItemExSetting*>();


void InjectInstructions();

HWND _CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	auto hwnd = RecottePluginFoundation::LookupFunction<decltype(&_CreateWindowExW)>("user32.dll", "CreateWindowExW")
		(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

	if (lpWindowName != nullptr && std::wstring(lpWindowName) == L"タイムライン")
	{
		InjectInstructions();
		TimelineWindow = hwnd;
		return hwnd;
	}
	
	if (TimelineWindow != nullptr && hWndParent == TimelineWindow)
	{
		auto index = 0;
		HWND prev = hwnd;
		while (nullptr != (prev = GetNextWindow(prev, GW_HWNDPREV))) index++;
		if (index == 1)
		{
			SetWindowTextW(hwnd, L"uh_Timeline_Labels");
			TimelineWidnowLabels = hwnd;
		}
		else if (index == 2)
		{
			SetWindowTextW(hwnd, L"uh_Timeline_Main");
			TimelineMainWidnow = hwnd; // WM_RBUTTONUPのコンテキストメニューにアクセスしたい…
		}
		return hwnd;
	}
	
	if (TimelineWidnowLabels != nullptr && hWndParent == TimelineWidnowLabels)
	{
		SetWindowTextW(hwnd, L"uh_Timeline_Labels_item");
		TimelineWidnowLabelsItems[hwnd] = new TimelineLabelItemExSetting{ .Hwnd = hwnd };
		return hwnd;
	}
	
	if (TimelineWidnowLabelsItems.contains(hWndParent))
	{
		auto index = 0;
		HWND prev = hwnd;
		while (nullptr != (prev = GetNextWindow(prev, GW_HWNDPREV))) index++;
		if(index == 1)
		{
			SetWindowTextW(hwnd, L"Timeline_Label_Box");
			TimelineWidnowLabelsItems[hWndParent]->BoxOriginalProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
			WNDPROC proc = [](HWND h, UINT u, WPARAM w, LPARAM l) -> LRESULT
			{
				if (u == WM_LBUTTONUP)
				{
					TimelineWidnowLabelsItems[GetParent(h)]->Folding = !TimelineWidnowLabelsItems[GetParent(h)]->Folding;
				}
				return TimelineWidnowLabelsItems[GetParent(h)]->BoxOriginalProc(h, u, w, l);
			};
			SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(proc));
		}
	}

	return hwnd;
}

void Hook_CalcLayerHeight(void* layerObj)
{
	auto windowObj = *(void**)((uint64_t)layerObj + 0xE8);
	auto hwnd = *(HWND*)((uint64_t)windowObj + 0x780);
	auto layerHeight = (float*)((uint64_t)layerObj + 0x160);
	OutputDebugStringW(std::format(L"[LayerFolding] OnHook HWND:0x{:X} H:{:f}\n", (uint64_t)hwnd, *layerHeight).c_str());
	
	auto additionalSetting = TimelineWidnowLabelsItems[hwnd];
	if (additionalSetting->Folding)
	{
		*layerHeight = 68.0f; // デフォルトの最小値
		UpdateWindow(hwnd);
	}
}

void InjectInstructions()
{
	// movssの次のleaの先頭アドレス
	auto injecteeAddress = (void*)0x00007FF6633A3898; // 48 8D 4C 24 70 E8 DE 37 F2 FF 90
	auto hookFunctionPtr = &Hook_CalcLayerHeight;
	unsigned char machineCode[22]
	{
	  0x48, 0x8B, 0xCF, // mov rcx, rdi
	  0x48, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // mov rax, 0FFFFFFFFFFFFFFFFh
	  0xFF, 0xD0, // call rax
	  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // nop x7
	};
	auto jmpAddress = (decltype(&Hook_CalcLayerHeight)*)&machineCode[3 + 2];
	*jmpAddress = hookFunctionPtr;
	DWORD oldProtection;
	VirtualProtect(injecteeAddress, _countof(machineCode), PAGE_EXECUTE_READWRITE, &oldProtection);
	memcpy(injecteeAddress, machineCode, _countof(machineCode));
}

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	OutputDebugStringW(L"[LayerFolding] OnPluginStart\n");
	RecottePluginFoundation::OverrideImportFunction("user32.dll", "CreateWindowExW", _CreateWindowExW);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	OutputDebugStringW(L"[LayerFolding] OnPluginFinish\n");
}
