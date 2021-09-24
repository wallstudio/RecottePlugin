#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <format>
#include <math.h>
#include <float.h>
#include <cinttypes>
#include "../HookHelper/HookHelper.h"
#include "RecotteObject.h"


decltype(&CreateWindowExW) g_Original_CreateWindowExW;
HWND TimelineWindow = nullptr;
HWND TimelineWidnowLabels = nullptr;
HWND TimelineMainWidnow = nullptr;


struct TimelineLabelItemExSetting
{
	HWND Hwnd;
	bool Folding;
	WNDPROC BoxOriginalProc;
	LayerObj* data;
};
std::map<HWND, TimelineLabelItemExSetting*> TimelineWidnowLabelsItems = std::map<HWND, TimelineLabelItemExSetting*>();
LayerObj* forceHitTestPass = nullptr;


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
			SetWindowTextW(hwnd, L"uh_timeline_labels");
			TimelineWidnowLabels = hwnd;
		}
		else if (index == 2)
		{
			SetWindowTextW(hwnd, L"uh_timeline_main");
			TimelineMainWidnow = hwnd; // WM_RBUTTONUPのコンテキストメニューにアクセスしたい…
		}
		return hwnd;
	}
	
	if (TimelineWidnowLabels != nullptr && hWndParent == TimelineWidnowLabels)
	{
		SetWindowTextW(hwnd, L"uh_timeline_labels_item");
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
			SetWindowTextW(hwnd, L"uh_timeline_label_box");
			TimelineWidnowLabelsItems[hWndParent]->BoxOriginalProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
			WNDPROC proc = [](HWND boxHwnd, UINT u, WPARAM w, LPARAM l) -> LRESULT
			{
				auto layer = TimelineWidnowLabelsItems[GetParent(boxHwnd)];
				if (u == WM_LBUTTONUP)
				{
					layer->Folding ^= true;

					POINT pos;
					GetCursorPos(&pos); // Yがレイヤー選択に使われるので

					forceHitTestPass = layer->data;
					SendMessageW(TimelineMainWidnow, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(0, pos.y));
					forceHitTestPass = nullptr;
					PostMessageW(TimelineMainWidnow, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(0, pos.y));
					PostMessageW(TimelineMainWidnow, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(0, pos.y));
				}
				else if (u == WM_PAINT)
				{
					PAINTSTRUCT ps;
					auto hdc = BeginPaint(boxHwnd, &ps);
					SetBkColor(hdc, RGB(0x40, 0x40, 0x40));
					SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
					auto markText = layer->Folding ? L" > " : L" v ";
					TextOutW(hdc, 0, 0, markText, lstrlenW(markText));
					EndPaint(boxHwnd, &ps);
					return 0;
				}
				return layer->BoxOriginalProc(boxHwnd, u, w, l);
			};
			SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(proc));
		}
	}

	return hwnd;
}


void Hook_CalcLayerHeight2(float xmm0, LayerObj* layerObj)
{
	if (layerObj == nullptr
		|| layerObj->window.get() == nullptr 
		|| layerObj->window->hwnd.get() == nullptr
		|| !TimelineWidnowLabelsItems.contains(layerObj->window->hwnd.get()))
	{
		return; // RSPの読み込みタイミングによって、CreateWindowより先に来ることがあるっぽい
	}

	if (layerObj->layerHeight.get() < xmm0)
	{
		layerObj->layerHeight = layerObj->getConstantMinHeight.get()();
	}

	auto additionalSetting = TimelineWidnowLabelsItems[layerObj->window->hwnd.get()];
	additionalSetting->data = layerObj;
	if (additionalSetting->Folding)
	{
		layerObj->layerHeight = 68.0f; // デフォルトの最小値
		UpdateWindow(layerObj->window->hwnd.get());
	}
}
void Hook_CalcLayerHeight(LayerObj* layerObj) { Hook_CalcLayerHeight2(-FLT_MAX, layerObj); }
void Hook_CalcLayerHeight3(float xmm0, LayerObj* layerObj) { Hook_CalcLayerHeight2(xmm0, layerObj); }


LayerObj::Object* Hook_HitTest(A1* a1, Vector2* click)
{
	if (forceHitTestPass != nullptr
		&& forceHitTestPass->objectCount.get() > 0)
	{
		return forceHitTestPass->objects.get()[0];
	}

	Vector2 timelineSize;
	a1->timeline->getSize.get()(a1->timeline.get(), &timelineSize);
	
	LayerObj* layer = nullptr;
	auto layerList = a1->unknown0->layerList.get();
	if ( layerList->leyerCount.get() <= 0 ) return nullptr;
	for (int i = layerList->leyerCount.get() - 1; i >= 0; i--)
	{
		layer = layerList->leyers.get()[i];
		if (layer->invalid.get()) continue;
		if ( click->y < layer->leyerMinY.get() || (layer->leyerMinY.get() + layer->layerHeight.get()) < click->y ) continue;
		break;
	}
	if (layer == nullptr) return nullptr;

	auto scale = layerList->scale.get();
	auto offset = a1->unknown1->offset.get();
	for(int i = layer->objectCount.get() - 1; i >= 0; i--) // 線形探索的な
	{
		auto target = layer->objects.get()[i];
		auto minX = target->getMin.get()(target) * scale - offset;
		auto maxX = target->getMax.get()(target) * scale - offset;
		if ( maxX < 0.0 || timelineSize.x < minX) continue; // オブジェクトが画面内かチェック
		
		auto x = target->getMin.get()(target) * scale - offset;
		auto w = target->getMax.get()(target) * scale - offset - x;
		auto y = target->y.get() + target->layerInfo->leyerMinY.get();
		auto h = target->h.get();
		if ( click->x < x || (x + w) < click->x ) continue; // HitTest X
		if ( click->y < y || (y + h) < click->y ) continue; // HitTest Y

		return target; // 見つかったど！
	}

	return nullptr;
}


LRESULT Hook_Dispatch(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LONG_PTR windowLongPtrW = Msg == 1
		? *(LONG_PTR*)lParam
		: GetWindowLongPtrW(hWnd, -21);

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

	// HitTest
	{
		auto target = RecottePluginFoundation::SearchAddress([](std::byte* address)
		{
			static unsigned char part0[] =
			{
				// function start
				// .text:00007FF6AEBB9600             HitTest proc near        
				0x48, 0x89, 0x5C, 0x24, 0x10, 				// mov [rsp+arg_8], rbx
				0x48, 0x89, 0x6C, 0x24, 0x18, 				// mov [rsp+arg_10], rbp
				0x48, 0x89, 0x74, 0x24, 0x20, 				// mov [rsp+arg_18], rsi
				0x57,										// push    rdi
				0x41, 0x56, 								// push    r14
				0x41, 0x57,									// push    r15
				0x48, 0x83, 0xEC, 0x50,						// sub     rsp, 50h
				// start main proc
				0x4C, 0x8B, 0xF1, 							// mov     r14, rcx
				0x0F, 0x29, 0x74, 0x24, 0x40, 				// movaps  [rsp+68h+var_28], xmm6
				0x48, 0x8B, 0x89, 0x68, 0x0C, 0x00, 0x00, 	// mov     rcx, [rcx+0C68h]
				0x48, 0x8B, 0xEA, 							// mov     rbp, rdx
				0x48, 0x8D, 0x54, 0x24, 0x70, 				// lea     rdx, [rsp+68h+timelineWidth]
				0x0F, 0x29, 0x7C, 0x24, 0x30, 				// movaps  [rsp+68h+var_38], xmm7
				0x48, 0x8B, 0x01, 							// mov     rax, [rcx]
				0xFF, 0x90, 0xB0, 0x00, 0x00, 0x00, 		// call    qword ptr [rax+0B0h]
				0x49, 0x8B, 0x86, 0xD0, 0x0A, 0x00, 0x00, 	// mov     rax, [r14+0AD0h]
				0x48, 0x8B, 0x80, 0x90, 0x0B, 0x00, 0x00, 	// mov     rax, [rax+0B90h]
				0x48, 0x63, 0x48, 0x38, 					// movsxd  rcx, dword ptr [rax+38h]
				0x85, 0xC9, 								// test    ecx, ecx
			};
			if (0 != memcmp(address, part0, sizeof(part0))) return false;
			address += sizeof(part0);
			return true;
		});
		unsigned char part3[]
		{
			0b0100'1'0'0'0, 0xB8 + 0b000,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // mov rax, 0FFFFFFFFFFFFFFFFh
			0xFF, 0b11'100'000, // jmp rax
			0x90, 0x90, 0x90, // nop
		};
		*(void**)&part3[2] = &Hook_HitTest;
		RecottePluginFoundation::MemoryCopyAvoidingProtection(target, part3, sizeof(part3));
	}

	// MessageDispacher（解析用）
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
