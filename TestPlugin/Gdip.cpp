#include <objbase.h>
#include <gdiplus.h>
#include "../HookHelper/HookHelper.h"
#include <vector>
#include <fmt/format.h>
#include "Gdip.h"
#include <gdiplustypes.h>

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

const char MODULE[] = "gdiplus.dll";


HWND GetHwnd(GpGraphics* graphics)
{
	std::wstring windowText;
	HDC hdc = nullptr;
	DllExports::GdipGetDC(graphics, &hdc);
	HWND hwnd = nullptr;
	if (hdc != nullptr)
	{
		hwnd = WindowFromDC(hdc);
		DllExports::GdipReleaseDC(graphics, hdc);
	}
	return hwnd;
}

extern "C" __declspec(dllexport) GpStatus WINGDIPAPI _GdipDrawString(
	GpGraphics * graphics,
	GDIPCONST WCHAR * string,
	INT                       length,
	GDIPCONST GpFont * font,
	GDIPCONST RectF * layoutRect,
	GDIPCONST GpStringFormat * stringFormat,
	GDIPCONST GpBrush * brush)
{
	if (std::wstring(string) == L"MAKIMAKI")
	{
		OutputDebugStringW(fmt::format(L"[TestPlugin] GdipDrawString: {} {},{} ({}x{}\n", string, layoutRect->X, layoutRect->Y, layoutRect->Width, layoutRect->Height).c_str());
		lstrcpyW((WCHAR*)string, L"マキマキ！！！！");
	}

	typedef GpStatus WINGDIPAPI GdipDrawString(
		GpGraphics* graphics,
		GDIPCONST WCHAR* string,
		INT                       length,
		GDIPCONST GpFont* font,
		GDIPCONST RectF* layoutRect,
		GDIPCONST GpStringFormat* stringFormat,
		GDIPCONST GpBrush* brush);
	auto base = RecottePluginFoundation::LookupFunction<GdipDrawString*>(MODULE, "GdipDrawString");
	if (base == nullptr) return GpStatus::GenericError;
	return base(graphics, string, length, font, layoutRect, stringFormat, brush);
}


extern "C" __declspec(dllexport) GpStatus WINGDIPAPI _GdipDrawRectangle(GpGraphics * graphics, GpPen * pen, REAL x, REAL y, REAL width, REAL height)
{
	//auto hwnd = GetHwnd(graphics); // どうもHWND非Bindっぽい
	typedef GpStatus WINGDIPAPI GdipDrawRectangle(GpGraphics* graphics, GpPen* pen, REAL x, REAL y, REAL width, REAL height);
	auto base = RecottePluginFoundation::LookupFunction<GdipDrawRectangle*>(MODULE, "GdipDrawRectangle");
	if (base == nullptr) return GpStatus::GenericError;
	return base(graphics, pen, x, y, width, height);
}


std::vector<FuncInfo> GetOverrideFunctions()
{
	return
	{
		FuncInfo{ MODULE, "GdipDrawString", _GdipDrawString },
		FuncInfo{ MODULE, "GdipDrawRectangle", _GdipDrawRectangle },
		// 使わないやつら
		//FuncInfo{ MODULE, "GdipDrawRectangleI", _GdipDrawRectangleI },
		//FuncInfo{ MODULE, "GdipDrawRectangles", _GdipDrawRectangles },
		//FuncInfo{ MODULE, "GdipDrawRectanglesI", _GdipDrawRectanglesI },
	};
}
