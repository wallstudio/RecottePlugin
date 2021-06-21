#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <format>
#include <math.h>
#include "../HookHelper/HookHelper.h"

#include <objbase.h>
#include <gdiplus.h>
#include <gdiplustypes.h>
#pragma comment(lib, "gdiplus.lib")

const char MODULE[] = "gdiplus.dll";


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


Gdiplus::GpStatus Hook_DrawTimeline_GdipGraphicsClear(Gdiplus::GpGraphics* graphics, Gdiplus::ARGB color)
{
	return Gdiplus::DllExports::GdipGraphicsClear(graphics, 0xffff1010);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	OutputDebugStringW(L"[CustomSkin] OnPluginStart\n");

	RecottePluginFoundation::InjectInstructions((void*)0x00007FF6634CE7D1, &Hook_DrawTimeline_GdipGraphicsClear, 2, std::vector<unsigned char>
	{
		0x48, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // mov rax, 0FFFFFFFFFFFFFFFFh
		0xFF, 0xD0, // call rax
		0x90, // nop x3
	});
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	OutputDebugStringW(L"[CustomSkin] OnPluginFinish\n");
}
