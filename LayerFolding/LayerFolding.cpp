#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <format>
#include <math.h>
#include "../HookHelper/HookHelper.h"


decltype(&CreateWindowExW) g_Original_CreateWindowExW;
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


HWND _CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	auto hwnd = g_Original_CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

	if (lpWindowName != nullptr && std::wstring(lpWindowName) == L"タイムライン")
	{
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


struct LayerObj
{
	struct
	{
		std::byte __Gap0[0x60];
		float(*GetConstantMinHeight)();
	}* UnknownObj;
	std::byte __Gap0[0xE8 - sizeof(UnknownObj)];
	struct
	{
		std::byte __Gap0[0x780];
		HWND Hwnd;
	}* WindowObj;

	std::byte __Gap1[0x160 - sizeof(UnknownObj) - sizeof(__Gap0) - sizeof(WindowObj)];
	float LayerHeight;
};

void Hook_CalcLayerHeight(LayerObj* layerObj)
{
	if (layerObj == nullptr
		|| layerObj->WindowObj == nullptr 
		|| layerObj->WindowObj->Hwnd == nullptr
		|| !TimelineWidnowLabelsItems.contains(layerObj->WindowObj->Hwnd))
	{
		return; // RSPの読み込みタイミングによって、CreateWindowより先に来ることがあるっぽい
	}

	auto additionalSetting = TimelineWidnowLabelsItems[layerObj->WindowObj->Hwnd];
	if (additionalSetting->Folding)
	{
		layerObj->LayerHeight = 68.0f; // デフォルトの最小値
		UpdateWindow(layerObj->WindowObj->Hwnd);
	}
}

void Hook_CalcLayerHeight2(float xmm0, LayerObj* layerObj)
{
	if (layerObj == nullptr
		|| layerObj->WindowObj == nullptr 
		|| layerObj->WindowObj->Hwnd == nullptr
		|| !TimelineWidnowLabelsItems.contains(layerObj->WindowObj->Hwnd))
	{
		return; // RSPの読み込みタイミングによって、CreateWindowより先に来ることがあるっぽい
	}

	if (layerObj->LayerHeight < xmm0)
	{
		layerObj->LayerHeight = layerObj->UnknownObj->GetConstantMinHeight();
	}

	auto additionalSetting = TimelineWidnowLabelsItems[layerObj->WindowObj->Hwnd];
	if (additionalSetting->Folding)
	{
		layerObj->LayerHeight = 68.0f; // デフォルトの最小値
		UpdateWindow(layerObj->WindowObj->Hwnd);
	}
}

void Hook_CalcLayerHeight3(float xmm0, LayerObj* layerObj) { Hook_CalcLayerHeight2(xmm0, layerObj); }

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	OutputDebugStringW(L"[LayerFolding] OnPluginStart\n");

	g_Original_CreateWindowExW = RecottePluginFoundation::OverrideIATFunction("user32.dll", "CreateWindowExW", _CreateWindowExW);

	// 話者レイヤー
	{
		auto target = RecottePluginFoundation::SearchAddress([](std::byte* address)
		{
			static auto part0 = std::vector<unsigned char>
			{
				0x48, 0x8D, 0x4C, 0x24, 0x70, // lea rcx, qword ptr [rsp+70h]
				0xE8, /* 0xDE, 0x37, 0xF2, 0xFF, */ // call 00007FF6632C7080h
			};
			if (0 != memcmp(address, part0.data(), part0.size())) return false;
			address += part0.size();
			address += sizeof(uint32_t);

			static auto part1 = std::vector<unsigned char>
			{
				0x90, // nop
				0x48, 0x8D, 0x4C, 0x24, 0x30, // lea rcx, qword ptr [rsp+30h]
				0xE8, /* 0xD3, 0x37, 0xF2, 0xFF, */ // call 00007FF6632C7080h
			};
			if (0 != memcmp(address, part1.data(), part1.size())) return false;
			address += part1.size();
			address += sizeof(uint32_t);

			static auto part2 = std::vector<unsigned char>
			{
				0x90, // nop
			};
			if (0 != memcmp(address, part2.data(), part2.size())) return false;
			address += part2.size();

			return true;
		});
		auto part3 = std::vector<unsigned char>
		{
			0x48, 0x8B, 0xCF, // mov rcx, rdi
			0x48, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // mov rax, 0FFFFFFFFFFFFFFFFh
			0xFF, 0xD0, // call rax
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // nop x7
		};
		*(void**)(part3.data() + 3 + 2) = &Hook_CalcLayerHeight;
		RecottePluginFoundation::MemoryCopyAvoidingProtection(target, part3.data(), part3.size());
	}

	// 注釈レイヤー
	{
		auto target = RecottePluginFoundation::SearchAddress([](std::byte* address)
		{
			static auto part0 = std::vector<unsigned char>
			{
				// 0x00007FF6A522AFDC
				0x0F, 0x2F, 0xF8, // comiss xmm7, xmm0
				0x76, 0x13, // jbe short loc_7FF6A522AFF4
				0xF3, 0x0F, 0x11, 0xBB, 0x60, 0x01, 0x00, 0x00, //movss dword ptr [rbx+160h], xmm7
				0x0F, 0x28, 0x7C, 0x24, 0x20, // movaps xmm7, [rsp+38h+var_18]
				0x48, 0x83, 0xC4, 0x30, // add rsp, 30h
				0x5B, // pop rbx
				0xC3, // retn
				// loc_7FF6A522AFF4:
				0x48, 0x8B, 0x03, // mov rax, [rbx]
				0x48, 0x8B, 0xCB, // mov rcx, rbx
				0xFF, 0x50, 0x60, // call qword ptr [rax+60h]
				0x0F, 0x28, 0x7C, 0x24, 0x20, // movaps xmm7, [rsp+38h+var_18]
				0xF3, 0x0F, 0x11, 0x83, 0x60, 0x01, 0x00, 0x00, // movss dword ptr [rbx+160h], xmm0
			};
			if (0 != memcmp(address, part0.data(), part0.size())) return false;
			address += part0.size();

			return true;
		});
		auto part3 = std::vector<unsigned char>
		{
			// dmmx0 がセット済み（第一引数）
			0x48, 0x8B, 0b11'010'011, // mov rdx, rbx （第二引数）
			0x48, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // mov rax, 0FFFFFFFFFFFFFFFFh
			0xFF, 0xD0, // call rax
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // nop
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // nop
		};
		*(void**)(part3.data() + 3 + 2) = &Hook_CalcLayerHeight2;
		RecottePluginFoundation::MemoryCopyAvoidingProtection(target, part3.data(), part3.size());
	}

	// 映像・音声レイヤー
	{
		auto target = RecottePluginFoundation::SearchAddress([](std::byte* address)
			{
				static auto part0 = std::vector<unsigned char> // 14
				{
					// 00007FF633B75F3A
					0xF3, 0x41, 0x0F, 0x11, 0x86, 0x60, 0x01, 0x00, 0x00, // movss dword ptr[r14 + 160h], xmm0
					0x48, 0x8D, 0x4D, 0xC0, // lea rcx,[rbp + 60h + var_A0]
					0xE8, // call ...
				};
				if (0 != memcmp(address, part0.data(), part0.size())) return false;
				address += part0.size();

				address += 4; // (call) sub_7FF633A273B0
				
				static auto part1 = std::vector<unsigned char> // 19
				{
					0x90, // nop
					0x48, 0x8D, 0x4C, 0x24, 0x70, // lea rcx,[rsp + 160h + var_F0]
				};
				if (0 != memcmp(address, part1.data(), part1.size())) return false;
				address += part1.size();

				return true;
			});
		auto part3 = std::vector<unsigned char>
		{
			// dmmx0 がセット済み（第一引数）
			0b0100'1'0'0'1 , 0x8B, 0b11'010'110, // mov rdx, r14（第二引数）REX(prefix, is64:t, shiftDst:f, shiftFactor:f, shiftSrc:t), mov, ModRM(mod:r, reg:rdx, rm:rsi/r14)
			0x48, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // mov rax, 0FFFFFFFFFFFFFFFFh
			0xFF, 0xD0, // call rax
			0x90, 0x90, 0x90, // nop
		};
		*(void**)(part3.data() + 3 + 2) = &Hook_CalcLayerHeight3;
		RecottePluginFoundation::MemoryCopyAvoidingProtection(target, part3.data(), part3.size());
	}
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	OutputDebugStringW(L"[LayerFolding] OnPluginFinish\n");
}
