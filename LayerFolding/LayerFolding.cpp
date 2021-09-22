#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <format>
#include <math.h>
#include <cinttypes>
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

LRESULT Hook_Dispatch(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LONG_PTR windowLongPtrW = Msg == 1
		? *(LONG_PTR*)lParam
		: GetWindowLongPtrW(hWnd, -21);

	wchar_t text[256];
	GetWindowTextW(hWnd, text, _countof(text));
	if (std::wstring(text) == L"uh_Timeline_Main")
	{
		OutputDebugStringW(L"uh_Timeline_Main\n");

		if (Msg == WM_LBUTTONDOWN)
		{
			OutputDebugStringW(L"LPU\n");
		}
	}

	if (!windowLongPtrW)
	{
		return DefWindowProcW(hWnd, Msg, wParam, lParam);
	}
	else
	{
		void* v10 = *(void**)windowLongPtrW;
		LRESULT v11 = 0;
		auto callback = (void(*)(LONG_PTR, HWND, UINT, WPARAM, LPARAM, LRESULT*)) * (void**)((size_t)v10 + 504);
		callback(windowLongPtrW, hWnd, Msg, wParam, lParam, &v11);
		return v11;
	}
}

struct Layer
{
	std::byte _dummy0[0x130];
	struct Object
	{
		struct
		{
			std::byte _dummy0[184];
			double (*getMin)(Object*);
			std::byte _dummy2[200 - sizeof(_dummy0) - sizeof(getMin)];
			double (*getMax)(Object*);
		}* v;
	}** objects;
	std::int32_t objectCount;
	std::byte _dummy1[0x379 - sizeof(_dummy0) - sizeof(objects) - sizeof(objectCount)];
	bool invalid;
};
struct V2 { float x; float y; };
struct Rect { float x; float y; float w; float h; };
std::int64_t (*ExtractRect)(size_t, Rect*, Layer::Object*);
void* Hook_HitTest(size_t a1, Layer* layer, float timelineWidth, V2* click)
{
	if ( layer->invalid ) return nullptr;

	for(int i = layer->objectCount - 1; i >= 0; i--) // 線形探索的な
	{
		auto target = layer->objects[i];
		auto scale = *(double*)(*(size_t*)(*(size_t*)(a1 + 2768) + 2960) + 1864);
		auto offset = (double)*(std::int32_t*)(*(size_t*)(a1 + 3200) + 2640);
		auto minX = target->v->getMin(target) * scale - offset;
    	auto maxX = target->v->getMax(target) * scale - offset;
		if ( maxX < 0.0 || timelineWidth < minX) continue; // オブジェクトが画面内かチェック
		
		Rect rect;
		ExtractRect(a1, &rect, target);
		if ( click->x < rect.x || (rect.x + rect.w) < click->x ) continue; // HitTest X
		if ( click->y < rect.y || (rect.y + rect.h) < click->y ) continue; // HitTest Y

		return target; // 見つかったど！
	}
	return nullptr;
}

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

	{
		auto target = RecottePluginFoundation::SearchAddress([](std::byte* address)
		{
			static unsigned char part0[] = {
				// loc_7FF7837F96BD:
				0x80, 0xBB, 0x79, 0x03, 0x00, 0x00, 0x00,   // cmp byte ptr [rbx+379h], 0
				0x75, 0xD1,    								// jnz short loc_7FF7837F9697
				0x8B, 0x83, 0x38, 0x01, 0x00, 0x00,	  		// mov eax, [rbx+138h]
				0x83, 0xE8, 0x01,   						// sub eax, 1
				0x48, 0x63, 0xF8,   						// movsxd rdi, eax
				0x78, 0xC3,    								// js short loc_7FF7837F9697
				0x0F, 0x57, 0xFF,   						// xorps xmm7, xmm7
				0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, // nop word ptr [rax+rax+00000000h]
				// loc_7FF7837F96E0:
			};
			if (0 != memcmp(address, part0, sizeof(part0))) return false;
			address += sizeof(part0);
			address += 0x00007FF7837F9782 - 0x00007FF7837F96E0;
			auto offsetExtractRect = *(std::int32_t*)(address + 1);
			address += 5;
			ExtractRect = (decltype(ExtractRect))(address + offsetExtractRect);
			return true;
		});
		unsigned char part3[]
		{
			0b0100'1'0'0'1, 0x8B, 0b11'001'110, // mov rcx r14 ; arg0 = a1
			0b0100'1'0'0'0, 0x8B, 0b11'010'011, // mov rdx rbx ; arg1 = layer
			0xF3, 0x0F, 0x10, 0b01'010'100, 0b00'100'100, 0x68+0x8, // movss xmm2 [rsp+rsp*0+68h+timelineWidth] ; arg2 = timelineWidth
			0b0100'1'1'0'0, 0x8B, 0b11'001'101, // mov r9 rbp ; arg3 = click
			0b0100'1'0'0'0, 0xB8 + 0b000,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // mov rax, 0FFFFFFFFFFFFFFFFh
			0xFF, 0xD0, // call rax
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // nop

			// 00007FF7837F96E0 ~ 00007FF7837F97D5 (F5)
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, // nop
		};
		*(void**)&part3[3 + 3 + 6 + 3 + 2] = &Hook_HitTest;
		RecottePluginFoundation::MemoryCopyAvoidingProtection(target, part3, sizeof(part3));
	}

	{
		auto target = RecottePluginFoundation::SearchAddress([](std::byte* address)
		{
			static auto part0 = std::vector<unsigned char>
			{
				// loc_7FF7836D0DD4:
				0x48, 0x85, 0xDB,                   // test    rbx, rbx
				0x75, 0x13,                         // jnz     short loc_7FF7836D0DEC
				0x4C, 0x8B, 0xCE,                   // mov     r9, rsi                     ; lParam
				0x4D, 0x8B, 0xC6,                   // mov     r8, r14                     ; wParam
				0x8B, 0xD5,                         // mov     edx, ebp                    ; Msg
				0x48, 0x8B, 0xCF,                   // mov     rcx, rdi                    ; hWnd
				0xFF, 0x15, 0x76, 0x2B, 0x4E, 0x00, // call    cs:DefWindowProcW
				0xEB, 0x32,                         // jmp     short loc_7FF7836D0E1E
				// loc_7FF7836D0DEC:
				0x48, 0x8B, 0x03,                   // mov     rax, [rbx]
				0x48, 0x8D, 0x4C, 0x24, 0x30,       // lea     rcx, [rsp+68h+var_38]
				0x48, 0x89, 0x4C, 0x24, 0x28,       // mov     [rsp+68h+var_40], rcx
				0x4D, 0x8B, 0xCE,                   // mov     r9, r14
				0x48, 0x8B, 0xCB,                   // mov     rcx, rbx
				0x48, 0xC7, 0x44, 0x24, 0x30, 0x00, 0x00, 0x00, 0x00, // mov     [rsp+68h+var_38], 0
				0x44, 0x8B, 0xC5,                   // mov     r8d, ebp
				0x48, 0x89, 0x74, 0x24, 0x20,       // mov     [rsp+68h+var_48], rsi
				0x48, 0x8B, 0xD7,                   // mov     rdx, rdi
				0xFF, 0x90, 0xF8, 0x01, 0x00, 0x00, // call    qword ptr [rax+1F8h]
				0x48, 0x8B, 0x44, 0x24, 0x30,       // mov     rax, [rsp+68h+var_38]
			};
			if (0 != memcmp(address, part0.data(), part0.size())) return false;
			address += part0.size();

			return true;
		});
		auto part3 = std::vector<unsigned char> // 74
		{
			0x4C, 0x8B, 0xCE, // mov r9, rsi; lParam
			0x4D, 0x8B, 0xC6, // mov r8, r14; wParam
			0x8B, 0xD5, // mov edx, ebp; Msg
			0x48, 0x8B, 0xCF, // mov rcx, rdi; hWnd
			0x48, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // mov rax, 0FFFFFFFFFFFFFFFFh
			0xFF, 0xD0, // call rax
			// 23
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // nop
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // nop
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // nop
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // nop
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // nop
			0x90, 0x90, 0x90, 0x90, // nop
		};
		*(void**)(part3.data() + 3 + 3 + 2 + 3 + 2) = &Hook_Dispatch;
		RecottePluginFoundation::MemoryCopyAvoidingProtection(target, part3.data(), part3.size());
	}
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	OutputDebugStringW(L"[LayerFolding] OnPluginFinish\n");
}
