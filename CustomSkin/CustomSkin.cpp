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
	auto result = Gdiplus::DllExports::GdipGraphicsClear(graphics, color);
	
	static Gdiplus::GpBitmap* bitmap = nullptr;
	if (bitmap == nullptr)
	{
		Gdiplus::DllExports::GdipCreateBitmapFromFile(L"C:\\Users\\huser\\Downloads\\stand_mk\\マキマキ立ち絵4_0001.png", &bitmap);
	}
	unsigned int srcW, srcH;
	Gdiplus::DllExports::GdipGetImageWidth(bitmap, &srcW);
	Gdiplus::DllExports::GdipGetImageHeight(bitmap, &srcH);
	Gdiplus::GpRectF dst;
	Gdiplus::DllExports::GdipGetClipBounds(graphics, &dst);
	auto w = dst.Height * (srcW / (float)srcH);
	Gdiplus::DllExports::GdipDrawImageRect(graphics, bitmap, dst.Width - w, 0, w, dst.Height);

	return result;
}

void Hook_DrawTimeline_DrawLayerFoundation(void** drawInfo, float* xywh)
{
	using namespace RecottePluginFoundation;

	auto graphics = (Gdiplus::GpGraphics**)drawInfo[29];
	auto result = (Gdiplus::GpStatus*)&drawInfo[30];
	auto fill = drawInfo[6];
	if (fill)
	{
		auto brush = *Offset<Gdiplus::GpSolidFill*>(*Offset<void*>(fill, 16), 8);
		Gdiplus::ARGB color;
		Gdiplus::DllExports::GdipGetSolidFillColor(brush, &color);
		color = (color & 0x00FFFFFF) | 0xAA000000; // スケスケ
		Gdiplus::GpSolidFill* myBrush;
		Gdiplus::DllExports::GdipCreateSolidFill(color, &myBrush);
		*result = Gdiplus::DllExports::GdipFillRectangle(*graphics, myBrush, xywh[0], xywh[1], xywh[2], xywh[3]);
		Gdiplus::DllExports::GdipDeleteBrush(myBrush);
	}
	auto noFill = drawInfo[5];
	if (noFill)
	{
		auto pen = **Offset<Gdiplus::GpPen**>(noFill, 16);
		*result = Gdiplus::DllExports::GdipDrawRectangle(*graphics, pen, xywh[0], xywh[1], xywh[2], xywh[3]);
	}
}

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	OutputDebugStringW(L"[CustomSkin] OnPluginStart\n");

	RecottePluginFoundation::InjectInstructions(
		(void*)0x00007FF6634CE7D1,
		&Hook_DrawTimeline_GdipGraphicsClear, 2,
		std::array<unsigned char, 13>
		{
			0x48, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // mov rax, 0FFFFFFFFFFFFFFFFh
			0xFF, 0xD0, // call rax
			0x90, // nops
		});

	RecottePluginFoundation::InjectInstructions(
		(void*)0x00007FF6634CEA45,
		&Hook_DrawTimeline_DrawLayerFoundation, 2,
		std::array<unsigned char, 20>
		{
			0x48, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // mov rax, 0FFFFFFFFFFFFFFFFh
			0xFF, 0xD0, // call rax
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // nops
		});
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	OutputDebugStringW(L"[CustomSkin] OnPluginFinish\n");
}
