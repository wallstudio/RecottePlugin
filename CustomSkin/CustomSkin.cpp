#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <filesystem>
#include <string>
#include <set>
#include <map>
#include <format>
#include <math.h>
#include "../RecottePluginManager/PluginHelper.h"

#include <objbase.h>
#include <gdiplus.h>
#include <gdiplustypes.h>
#pragma comment(lib, "gdiplus.lib")


const auto EMSG_NOT_FOUND_DEFAULT_SKIN_RSP = L"デフォルトのSkinが見つけられません\r\n{}";
const auto EMSG_NOT_FOUND_RSP_PACKER = L"RSPファイルの展開プログラムが見つけられません\r\n{}";
const auto EMSG_FILED_UNPACK = L"RSPファイルの展開に失敗しました\r\n{}";
const auto EMSG_FILED_COMPOSITE = L"RSPファイルの合成に失敗しました\r\n{}";
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
				file = tmpDir / "embedded_skin.png";
				if (!std::filesystem::exists(file))
				{
					// https://github.com/wallstudio/Png2RspConverter/actions
					auto packer = RecottePluginManager::ResolvePluginPath() / "Png2RspConverter.exe";
					if (!std::filesystem::exists(rsp)) throw std::format(EMSG_NOT_FOUND_RSP_PACKER, packer.wstring());

					auto unpackCommand = std::format("\"{}\" --unpack \"{}\" \"{}\"", packer.string(), rsp.string(), tmpDir.string());
					OutputDebugStringA(std::format("{}\n", unpackCommand).c_str());
					auto unpackResult = std::system(std::format("\"{}\"", unpackCommand).c_str()); // 全体を更に引用符で囲う必要がある
					if (unpackResult != 0) throw std::format(EMSG_FILED_UNPACK, RecottePluginManager::AsciiToWide(unpackCommand));

					auto compCommand = std::format("\"{}\" --composite \"{}\" \"{}\" \"{}\" \"{}\"",
						packer.string(), file.string(),
						(tmpDir / "default_n.png.rdi").string(), (tmpDir / "action10_n.png.rdi").string(), (tmpDir / "action10_o.png.rdi").string());
					OutputDebugStringA(std::format("{}\n", compCommand).c_str());
					auto compResult = std::system(std::format("\"{}\"", compCommand).c_str()); // 全体を更に引用符で囲う必要がある
					if (compResult != 0) throw std::format(EMSG_FILED_COMPOSITE, RecottePluginManager::AsciiToWide(compCommand));
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
	using namespace RecottePluginManager::Instruction;
	{
		auto target = RecottePluginManager::SearchAddress([&](std::byte* address)
		{
			static unsigned char part0[] =
			{
				0xFF, 0x15, /* 0xA1, 0xD4, 0x2F, 0x00, */ // call cs:GdipGraphicsClear
			};
			if (0 != memcmp(address, part0, sizeof(part0))) return false;
			address += sizeof(part0);
			address += sizeof(uint32_t);

			static unsigned char part1[] =
			{
				0x85, 0xC0, // test eax, eax
				0x74, 0x03, // jz short loc_7FF6634CE7DE
				0x89, 0x46, 0x08, // mov [rdi+8], eax
				0x8B, 0x43, 0x10, // mov eax,[rbx + 10h]
			};
			if (0 != memcmp(address, part1, sizeof(part1))) return false;
			address += sizeof(part1);

			return true;
		});
		unsigned char part3[] =
		{
			REX(true, false, false, false), 0xB8 + Reg32::a, DUMMY_ADDRESS, // mov rax, 0FFFFFFFFFFFFFFFFh
			0xFF, ModRM(2, Mode::reg, Reg32::a), // call rax
			NOP, // nops
		};
		*(void**)(&part3[2]) = &Hook_DrawTimeline_GdipGraphicsClear;
		RecottePluginManager::MemoryCopyAvoidingProtection(target, part3, sizeof(part3));
	}

	{
		static unsigned char part0[] =
		{
			0x90, // nop
			0x48, 0x8D, 0x85, 0xD0, 0x01, 0x00, 0x00, // lea rax, [rbp + 340h + var_170]
			0x48, 0x89, 0x47, 0x30, // mov [rdi + 30h], rax
			0x48, 0x8D, 0x55, 0x20, // lea rdx, [rbp + 340h + var_320]
			0x48, 0x8B, 0xCF, // mov rcx, rdi
		};
		auto target = RecottePluginManager::SearchAddress([&](std::byte* address)
		{
			if (0 != memcmp(address, part0, sizeof(part0))) return false;
			address += sizeof(part0);

			static unsigned char part1[] =
			{
				0xE8, /* 0x77, 0x7B, 0xE2, 0xFF, */ // call DrawRectangle
			};
			if (0 != memcmp(address, part1, sizeof(part1))) return false;
			address += sizeof(part1);
			address += sizeof(uint32_t);

			static unsigned char part2[] =
			{
				0x90, // nop
				0x48, 0x8D, 0x05, /* 0xE7, 0x4A, 0x3B, 0x00, */ // lea rax, off_7FF6A569B008
			};
			if (0 != memcmp(address, part2, sizeof(part2))) return false;
			address += sizeof(part2);

			auto relativeAddressOffset = *((uint32_t*)address); // 0x0038C5B6
			address += sizeof(uint32_t);

			// 土地が足りないので、Global変数へのアクセスを肩代わりして稼ぐ
			Global_0 = address + relativeAddressOffset; // 7FF6A569B008; Capture
			return true;
		});
		target += sizeof(part0); // part0 はそのままに、part1, part2 を上書きする
		unsigned char  part3[] =
		{
			REX(true, false, false, false), 0xB8 + Reg32::a, DUMMY_ADDRESS, // mov rax, 0FFFFFFFFFFFFFFFFh
			0xFF, ModRM(2, Mode::reg, Reg32::a), // call rax
			NOP, // nops
		};
		*(void**)(&part3[2]) = &Hook_DrawTimeline_DrawLayerFoundation;
		RecottePluginManager::MemoryCopyAvoidingProtection(target, part3, sizeof(part3));
	}
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld) {}
