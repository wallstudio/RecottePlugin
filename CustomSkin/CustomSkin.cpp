﻿#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <filesystem>
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

void* Global_0;

Gdiplus::GpStatus Hook_DrawTimeline_GdipGraphicsClear(Gdiplus::GpGraphics* graphics, Gdiplus::ARGB color)
{
	auto result = Gdiplus::DllExports::GdipGraphicsClear(graphics, color);
	
	static Gdiplus::GpBitmap* bitmap = nullptr;
	if (bitmap == nullptr)
	{
		auto exePathBuffer = std::vector<wchar_t>(_MAX_PATH);
		GetModuleFileNameW(GetModuleHandleW(NULL), exePathBuffer.data(), exePathBuffer.size());
		auto exeDir = std::filesystem::path(exePathBuffer.data()).parent_path().string();
		auto pluginDir = std::filesystem::path(exeDir).append("Plugins").string();

		auto file = std::filesystem::path(pluginDir).append("skin.png").wstring();
		if (!std::filesystem::exists(std::filesystem::path(file)))
		{
			auto rsp = std::filesystem::path(exeDir).append("models").append("2D-Maki_shihuku.rsp").string();
			auto packer = std::filesystem::path(pluginDir).append("Png2RspConverter.exe").string();
			auto tmp = std::filesystem::temp_directory_path().append("RecotteStudioPlugin").append(std::filesystem::path(rsp).filename().string()).string();
			auto command = std::format("\"    \"{}\" --unpack \"{}\" \"{}\"    \"", packer, rsp, tmp);
			OutputDebugStringA(std::format("{}\n", command).c_str());
			auto result = std::system(command.c_str());
			file = std::filesystem::path(tmp).append("action10_o.png").wstring();
		}
		Gdiplus::DllExports::GdipCreateBitmapFromFile(file.c_str(), &bitmap);
	}
	unsigned int srcW, srcH;
	Gdiplus::DllExports::GdipGetImageWidth(bitmap, &srcW);
	Gdiplus::DllExports::GdipGetImageHeight(bitmap, &srcH);
	Gdiplus::GpRectF dst;
	Gdiplus::DllExports::GdipGetClipBounds(graphics, &dst);
	dst.Height -= 25; // 目盛りの分
	auto w = dst.Height * (srcW / (float)srcH);
	Gdiplus::DllExports::GdipDrawImageRect(graphics, bitmap, dst.Width - w, 25, w, dst.Height);

	return result;
}

uint64_t Hook_DrawTimeline_DrawLayerFoundation(void** drawInfo, float* xywh)
{
	using namespace RecottePluginFoundation;

	auto graphics = (Gdiplus::GpGraphics**)drawInfo[29];
	auto result = Offset<Gdiplus::GpStatus>(drawInfo[29], 8);
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
	return *(uint64_t*)Global_0;
}

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	OutputDebugStringW(L"[CustomSkin] OnPluginStart\n");

	// TODO: アドレスを含まないMarkerに
	RecottePluginFoundation::InjectInstructions(
		&Hook_DrawTimeline_GdipGraphicsClear, 2,
		std::array<unsigned char, 13>
		{
			// 0x00007FF6634CE7D1
			0xFF, 0x15, 0xA1, 0xD4, 0x2F, 0x00, // call cs:GdipGraphicsClear
			0x85, 0xC0, // test eax, eax
			0x74, 0x03, // jz short loc_7FF6634CE7DE
			0x89, 0x47, 0x08, // mov [rdi+8], eax
		},
		std::array<unsigned char, 13>
		{
			0x48, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // mov rax, 0FFFFFFFFFFFFFFFFh
			0xFF, 0xD0, // call rax
			0x90, // nops
		});

	{
		// 土地が足りないので、Global変数へのアクセスを肩代わりして稼ぐ
		auto marker = std::array <unsigned char, 19>
		{
			0x90, // nop
			0x48, 0x8D, 0x85, 0xB0, 0x01, 0x00, 0x00, // lea rax, [rbp + 320h + var_170]
			0x48, 0x89, 0x46, 0x30, // mov[rsi + 30h], rax
			0x48, 0x8D, 0x55, 0x18, // lea rdx, [rbp + 320h + var_308]
			0x48, 0x8B, 0xCE, // mov rcx, rsi
		};
		auto markerAddress = RecottePluginFoundation::SearchAddress(marker);
		auto injecteeAddress = markerAddress + marker.size();
		// E8 A6 D1 E4 FF          call DrawRectangle
		// 90                      nop
		// 48 8D 05 B6 C5 38 00    lea rax, off_7FF6A569B008
		auto offset = (uint32_t*)(injecteeAddress + 9); // 0x0038C5B6
		Global_0 = injecteeAddress + 13 + *offset;
		RecottePluginFoundation::InjectInstructions(
			injecteeAddress,
			&Hook_DrawTimeline_DrawLayerFoundation, 2,
			std::array<unsigned char, 13>
			{
				0x48, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // mov rax, 0FFFFFFFFFFFFFFFFh
				0xFF, 0xD0, // call rax
				0x90, // nop
			});
	}
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	OutputDebugStringW(L"[CustomSkin] OnPluginFinish\n");
}
