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


const auto EMSG_NOT_FOUND_DEFAULT_SKIN_RSP = L"デフォルトのSkinが見つけられません\r\n{}";
const auto EMSG_NOT_FOUND_RSP_PACKER = L"RSPファイルの展開プログラムが見つけられません\r\n{}";
const auto EMSG_FILED_UNPACK = L"RSPファイルの展開に失敗しました\r\n{}";
const auto EMSG_NOT_FOUND_SKIN_FILE = L"Skin用画像が見つけられません\r\n{}";
const auto EMSG_FAILED_LOAD_IMAGE = L"画像ファイルの読み込みに失敗しました\r\n{}";


void* Global_0;

Gdiplus::GpStatus Hook_DrawTimeline_GdipGraphicsClear(Gdiplus::GpGraphics* graphics, Gdiplus::ARGB color)
{
	auto result = Gdiplus::DllExports::GdipGraphicsClear(graphics, color);
	
	static Gdiplus::GpBitmap* bitmap = nullptr;
	if (bitmap == nullptr)
	{
		try
		{
			static auto pluginDir = RecottePluginManager::ResolvePluginPath();
			auto file = pluginDir / "skin.png";
			if (!std::filesystem::exists(file))
			{
				static auto rsp = RecottePluginManager::ResolveApplicationDir() / "models" / "2D-Maki_shihuku.rsp";
				if (!std::filesystem::exists(rsp)) throw std::format(EMSG_NOT_FOUND_DEFAULT_SKIN_RSP, rsp.wstring());

				auto tmpDir = std::filesystem::temp_directory_path() / "RecottePlugin" / rsp.filename();
				file = tmpDir / "action10_o.png";
				if (!std::filesystem::exists(file))
				{
					auto packer = RecottePluginManager::ResolvePluginPath() / "Png2RspConverter.exe";
					if (!std::filesystem::exists(rsp)) throw std::format(EMSG_NOT_FOUND_RSP_PACKER, packer.wstring());

					auto command = std::format("\"{}\" --unpack \"{}\" \"{}\"", packer.string(), rsp.string(), tmpDir.string());
					OutputDebugStringA(std::format("{}\n", command).c_str());
					auto result = std::system(std::format("\"{}\"", command).c_str()); // 全体を更に引用符で囲う必要がある
					if (result != 0) throw std::format(EMSG_FILED_UNPACK, RecottePluginManager::AsciiToWide(command));
				}
			}
			if (!std::filesystem::exists(file)) throw std::format(EMSG_NOT_FOUND_SKIN_FILE, file.wstring());

			auto status = Gdiplus::DllExports::GdipCreateBitmapFromFile(file.c_str(), &bitmap);
			if (status != Gdiplus::Status::Ok) throw std::format(EMSG_FAILED_LOAD_IMAGE, file.wstring());
		}
		catch (std::wstring& e)
		{
			MessageBoxW(nullptr, e.c_str(), L"RecottePlugin", MB_ICONERROR);
			exit(-1145141919); // 強制クラッシュ
		}
		catch (std::exception& e)
		{
			MessageBoxA(nullptr, e.what(), "RecottePlugin", MB_ICONERROR);
			exit(-1145141919); // 強制クラッシュ
		}
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
	using namespace RecottePluginManager;

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
	{
		auto target = RecottePluginManager::SearchAddress([&](std::byte* address)
		{
			static auto part0 = std::vector<unsigned char>
			{
				0xFF, 0x15, /* 0xA1, 0xD4, 0x2F, 0x00, */ // call cs:GdipGraphicsClear
			};
			if (0 != memcmp(address, part0.data(), part0.size())) return false;
			address += part0.size();
			address += sizeof(uint32_t);

			static auto part1 = std::vector<unsigned char>
			{
				0x85, 0xC0, // test eax, eax
				0x74, 0x03, // jz short loc_7FF6634CE7DE
				0x89, 0x47, 0x08, // mov [rdi+8], eax
				0x44, 0x8B, 0x73, 0x10, // mov r14d,[rbx + 10h]
			};
			if (0 != memcmp(address, part1.data(), part1.size())) return false;
			address += part1.size();

			return true;
		});
		auto part3 = std::vector<unsigned char>
		{
			0x48, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // mov rax, 0FFFFFFFFFFFFFFFFh
			0xFF, 0xD0, // call rax
			0x90, // nops
		};
		*(void**)(part3.data() + 2) = &Hook_DrawTimeline_GdipGraphicsClear;
		RecottePluginManager::MemoryCopyAvoidingProtection(target, part3.data(), part3.size());
	}

	{
		static auto part0 = std::vector<unsigned char>
		{
			0x90, // nop
			0x48, 0x8D, 0x85, 0xB0, 0x01, 0x00, 0x00, // lea rax, [rbp + 320h + var_170]
			0x48, 0x89, 0x46, 0x30, // mov[rsi + 30h], rax
			0x48, 0x8D, 0x55, 0x18, // lea rdx, [rbp + 320h + var_308]
			0x48, 0x8B, 0xCE, // mov rcx, rsi
		};
		auto target = RecottePluginManager::SearchAddress([&](std::byte* address)
		{
			if (0 != memcmp(address, part0.data(), part0.size())) return false;
			address += part0.size();

			static auto part1 = std::vector<unsigned char>
			{
				0xE8, /* 0xA6, 0xD1, 0xE4, 0xFF, */ // call DrawRectangle
			};
			if (0 != memcmp(address, part1.data(), part1.size())) return false;
			address += part1.size();
			address += sizeof(uint32_t);

			static auto part2 = std::vector<unsigned char>
			{
				0x90, // nop
				0x48, 0x8D, 0x05, /* 0xB6, 0xC5, 0x38, 0x00, */ // lea rax, off_7FF6A569B008
			};
			if (0 != memcmp(address, part2.data(), part2.size())) return false;
			address += part2.size();

			auto relativeAddressOffset = *((uint32_t*)address); // 0x0038C5B6
			address += sizeof(uint32_t);

			// 土地が足りないので、Global変数へのアクセスを肩代わりして稼ぐ
			Global_0 = address + relativeAddressOffset; // 7FF6A569B008; Capture
			return true;
		});
		// part0 はそのままに、part1, part2 を上書きする
		target += part0.size();
		auto part3 = std::vector<unsigned char>
		{
			0x48, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // mov rax, 0FFFFFFFFFFFFFFFFh
			0xFF, 0xD0, // call rax
			0x90, // nop
		};
		*(void**)(part3.data() + 2) = &Hook_DrawTimeline_DrawLayerFoundation;
		RecottePluginManager::MemoryCopyAvoidingProtection(target, part3.data(), part3.size());
	}
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld) {}
