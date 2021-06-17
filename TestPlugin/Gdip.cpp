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
	HDC hdc = nullptr;
	DllExports::GdipGetDC(graphics, &hdc);
	if (hdc == nullptr) return nullptr;
	HWND hwnd = WindowFromDC(hdc);
	DllExports::GdipReleaseDC(graphics, hdc);
	if (hwnd == nullptr) return nullptr;
	return hwnd;
}

__declspec(dllexport) GpStatus _GdipDrawString(GpGraphics* graphics, WCHAR* string, INT length, GpFont* font, RectF* layoutRect, GpStringFormat* stringFormat, GpBrush* brush)
{
	if (std::wstring(string) == L"MAKIMAKI")
	{
		// OutputDebugStringW(fmt::format(L"[TestPlugin] GdipDrawString: {} {},{} ({}x{}\n", string, layoutRect->X, layoutRect->Y, layoutRect->Width, layoutRect->Height).c_str());
		lstrcpyW((WCHAR*)string, L"マキマキ！！！！");
	}
	return RecottePluginFoundation::LookupFunction<decltype(&_GdipDrawString)>(MODULE, "GdipDrawString")(graphics, string, length, font, layoutRect, stringFormat, brush);
}

__declspec(dllexport) GpStatus _GdipDrawRectangle(GpGraphics * graphics, GpPen * pen, REAL x, REAL y, REAL width, REAL height)
{
	if ((152.0 > width && width > 147.0) && (27.0 > height && height > 23.0))
	{
		// OutputDebugStringW(fmt::format(L"[TestPlugin] GdipDrawRectangle: {},{} ({}x{})\n", x, y, width, height).c_str());
		return GpStatus::Ok;
	}
	return RecottePluginFoundation::LookupFunction<decltype(&_GdipDrawRectangle)>(MODULE, "GdipDrawRectangle")(graphics, pen, x, y, width, height);
}

__declspec(dllexport) GpStatus _GdipFillRectangle(GpGraphics * graphics, GpBrush * brush, REAL x, REAL y, REAL width, REAL height)
{
	auto hwnd = GetHwnd(graphics); // どうもHWND非Bindっぽい
	auto base = RecottePluginFoundation::LookupFunction<decltype(&_GdipFillRectangle)>(MODULE, "GdipFillRectangle");
	// OutputDebugStringW(fmt::format(L"[TestPlugin] GdipFillRectangle: {},{} ({}x{})\n", x, y, width, height).c_str());
	if ((152.0 > width && width > 147.0) && (27.0 > height && height == 23.0)) return GpStatus::Ok;
	return base(graphics, brush, x, y, width, height);
}

__declspec(dllexport) GpStatus __GdipDrawLine(GpGraphics * graphics, GpPen * pen, REAL x1, REAL y1, REAL x2, REAL y2)
{
	return RecottePluginFoundation::LookupFunction<decltype(&__GdipDrawLine)>(MODULE, "GdipDrawLine")(graphics, pen, x1, y1, x2, y2);
}

_declspec(dllexport) void* __fastcall GetLayer(void* v, int index)
{
	auto base = (void* (__fastcall*)(void*, int))(0x00007FF79ACA9EC0);
	auto layer = base(v, index);
	return layer;
}


std::vector<FuncInfo> GetOverrideFunctions()
{
	//{
	//	auto baseRSDrawControll = (uint8_t*)0x7FF79AD341A0;
	//	DWORD oldProtection;
	//	VirtualProtect(baseRSDrawControll, 1, PAGE_EXECUTE_READWRITE, &oldProtection);
	//	*baseRSDrawControll = 0xC3; // ret
	//}
	//{
	//	auto baseRSDrawControllLoop = (uint8_t*)0x7FF79AE09D70;
	//	DWORD oldProtection;
	//	VirtualProtect(baseRSDrawControllLoop, 1, PAGE_EXECUTE_READWRITE, &oldProtection);
	//	*baseRSDrawControllLoop = 0xC3; // ret
	//}
	//	
	//{
	//	auto startGetLayer = 0x00007FF79ACA9EC0;
	//	auto endGetLayer = 0x00007FF79ACAA020;
	//	auto size = endGetLayer - startGetLayer;
	//	DWORD oldProtection;
	//	VirtualProtect((void*)startGetLayer, size, PAGE_EXECUTE_READWRITE, &oldProtection);
	//	auto backupGetLayer = (void*(_fastcall*)(void*, int))VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	//	memcpy(backupGetLayer, (void*)startGetLayer, size);
	//	auto jumpInst = std::vector<byte>();
	//	memcpy((void*)startGetLayer, jumpInst.data(), jumpInst.size());
	//}
	{
		//auto callBaseLayer = (uint8_t*)0x00007FF79AE09F82;
		//DWORD oldProtection;
		//VirtualProtect(callBaseLayer, 1, PAGE_EXECUTE_READWRITE, &oldProtection);
		//callBaseLayer[0] = 0xC3; // ret
	}
	return
	{
		FuncInfo{ MODULE, "GdipDrawString", _GdipDrawString },
		FuncInfo{ MODULE, "GdipDrawRectangle", _GdipDrawRectangle },
		FuncInfo{ MODULE, "GdipFillRectangle", _GdipFillRectangle },
		FuncInfo{ MODULE, "GdipDrawLine", __GdipDrawLine },
	};
}
