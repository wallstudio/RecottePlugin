#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <map>
#include <filesystem>
#include "coreclr_delegates.h"
#include "hostfxr.h"
#include "resource.h"
#include "../RecottePluginManager/PluginHelper.h"


const auto EMSG_FAILED_PLUGIN_DLL = L"プラグインDLLが読み込めません\r\n{}\r\n{}";
const auto EMSG_FAILED_PLUGIN_STARR = L"プラグインの開始ができません\r\n{}\r\n{}";
const auto EMSG_FAILED_PLUGIN_FINISH = L"プラグインの終了ができません\r\n{}\r\n{}";



HINSTANCE hLibMine;
HINSTANCE hLib;
FARPROC p[51];

auto g_Plugins = std::map<std::filesystem::path, HINSTANCE>();


inline HWND _CreateWindowExW(decltype(&CreateWindowExW) base, DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	auto hwnd = base(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

	if (lpWindowName == nullptr || std::wstring(lpWindowName) != L"Update") return hwnd;
	
	auto parent = GetParent(hwnd);
	if (parent == nullptr) return hwnd;

	auto parntParent = GetParent(parent);
	if (parent == nullptr) return hwnd;

	static auto text = std::vector<wchar_t>(512);
	GetWindowTextW(parntParent, text.data(), text.size());
	if (std::wstring(text.data()) != L"Recotte Studio") return hwnd;

	// PreviewWidnowの機能ボタン
	{
		auto hwnd = base(dwExStyle, L"BUTTON", L"uh_plugin_config", dwStyle, 0, 0, 28, 28, hWndParent, hMenu, hInstance, lpParam);
		WNDPROC proc = [](HWND hwnd, UINT m, WPARAM w, LPARAM l) -> LRESULT
		{
			static auto buttonPopupIcon = LoadIconW(GetModuleHandleW(L"d3d11"), MAKEINTRESOURCE(BUTTON_CONFIG));
			static auto background = CreateSolidBrush(0x202020);
			static auto hoverBackground = CreateSolidBrush(0x505050);
			static auto pressedBackground = CreateSolidBrush(0x202020);
			static auto hover = false;
			static auto pressed = false;
			switch (m)
			{
			case WM_LBUTTONUP:
			{
				auto text = std::wstring(L"以下のPluginがロードされています\r\n\r\n");
				for (auto [path, handle] : g_Plugins)
				{
					text += path.stem().wstring() + L"\r\n";
				}
				MessageBoxW(nullptr, text.c_str(), L"RecottePlugin", MB_OK);
				break;
			}
			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				auto hdc = BeginPaint(hwnd, &ps);
				static RECT rect = { 0, 0, 28, 28 };
				FillRect(hdc, &rect, pressed ? pressedBackground : (hover ? hoverBackground : background));
				auto u = DrawIconEx(hdc, 0, 0, buttonPopupIcon, 28, 28, DI_DEFAULTSIZE, nullptr, DI_NORMAL | DI_COMPAT);
				EndPaint(hwnd, &ps);
			}
			break;
			case WM_LBUTTONDOWN:
			case WM_MOUSEMOVE:
			{
				pressed = w == MK_LBUTTON;
				TRACKMOUSEEVENT track = {
					.cbSize = sizeof(TRACKMOUSEEVENT),
					.dwFlags = TME_HOVER | TME_LEAVE,
					.hwndTrack = hwnd,
					.dwHoverTime = 1,
				};
				TrackMouseEvent(&track);
				RedrawWindow(hwnd, nullptr, nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE);
			}
			break;
			case WM_MOUSEHOVER:
				hover = true;
				RedrawWindow(hwnd, nullptr, nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE);
				break;
			case WM_MOUSELEAVE:
				hover = pressed = false;
				RedrawWindow(hwnd, nullptr, nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE);
				break;
			case WM_DESTROY:
				DeleteObject(buttonPopupIcon);
				DeleteObject(background);
				DeleteObject(hoverBackground);
				DeleteObject(pressedBackground);
				break;
			}
			return DefWindowProcW(hwnd, m, w, l);
		};
		SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)proc);
		//SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		ShowWindow(hwnd, SW_SHOW);
	}

	return hwnd;
}

void LoadDotNetLibs()
{
	// STEP 1: Load HostFxr and get exported hosting functions
	auto lib = LoadLibraryW(L"C:\\Users\\huser\\Desktop\\mandll\\dotnet-runtime-5.0.10-win-x64\\host\\fxr\\5.0.10\\hostfxr.dll");
	auto init_fptr = (hostfxr_initialize_for_runtime_config_fn)GetProcAddress(lib, "hostfxr_initialize_for_runtime_config");
	auto get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)GetProcAddress(lib, "hostfxr_get_runtime_delegate");
	auto close_fptr = (hostfxr_close_fn)GetProcAddress(lib, "hostfxr_close");

	// STEP 2: Initialize and start the .NET Core runtime and Get the load assembly function pointer
	load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = nullptr;
	hostfxr_handle cxt = nullptr;
	init_fptr(L"C:\\Users\\huser\\Desktop\\mandll\\DotNetLib.runtimeconfig.json", nullptr, &cxt);
	get_delegate_fptr(
		cxt,
		hostfxr_delegate_type::hdt_load_assembly_and_get_function_pointer,
		(void**)&load_assembly_and_get_function_pointer);
	close_fptr(cxt);

	// STEP 3: Load managed assembly and Get function pointer to methods
	component_entry_point_fn hello = nullptr;
	load_assembly_and_get_function_pointer(
		L"C:\\Users\\huser\\Desktop\\mandll\\DotNetLib.dll",
		L"DotNetLib.Lib, DotNetLib",
		L"Hello",
		nullptr /*delegate_type_name*/,
		nullptr,
		(void**)&hello);
	struct lib_args { const char_t* message; int number; };
	void (*custom)(lib_args args) = nullptr;
	load_assembly_and_get_function_pointer(
		L"C:\\Users\\huser\\Desktop\\mandll\\DotNetLib.dll",
		L"DotNetLib.Lib, DotNetLib",
		L"CustomEntryPointUnmanaged" /*method_name*/,
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		(void**)&custom);

	// STEP 4: Run methods
	for (int i = 0; i < 3; ++i)
	{
		lib_args args{ L"from host!", i };
		hello(&args, sizeof(args)); // manual marshal
		custom(args); // auto marshal
	}

	OutputDebugStringW(std::format(L"{} {} {} {} {}",
		(void*)init_fptr, (void*)get_delegate_fptr, (void*)close_fptr, (void*)hello, (void*)custom).c_str());
}

int __WinMain(decltype(&WinMain) base, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	//LoadDotNetLibs();
	return base(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
}

void Hook_WinMain()
{
	using namespace RecottePluginManager::Instruction;

	OutputDebugStringW(std::format(L"a").c_str());

	static decltype(&WinMain) winMain = nullptr;
	auto target = RecottePluginManager::SearchAddress([&](std::byte* address)
	{
		static unsigned char part0[] =
		{
			0x4C, 0x8B, 0xC0,                               // mov r8, rax ; lpCmdLine
			0x44, 0x8B, 0xCB,                               // mov r9d, ebx ; nShowCmd
			0x33, 0xD2,                                     // xor edx, edx ; hPrevInstance
			0x48, 0x8D, 0x0D, /* 0x1F, 0x1F, 0xBD, 0xFF, */ // lea rcx, cs:140000000h ; hInstance
		};
		if (0 != memcmp(address, part0, sizeof(part0))) return false;
		address += sizeof(part0);
		auto winMainOffset = *(std::uint32_t*)address;
		address += sizeof(std::uint32_t);

		static unsigned char part1[] =
		{
			0xE8, 0x9A, 0xEB, 0xC2, 0xFF,                   // call WinMain
		};
		address += sizeof(part1);
		winMain = RecottePluginManager::Offset<decltype(WinMain)>(address, winMainOffset);

		return true;
	});
	OutputDebugStringW(std::format(L"{}", (void*)target).c_str());
	unsigned char part3[] =
	{
		REX(true, false, false, false), 0xB8 + Reg32::a, DUMMY_ADDRESS, // mov rax, 0FFFFFFFFFFFFFFFFh
		0xFF, ModRM(2, Mode::reg, Reg32::a), // call rax
		NOP, // nops
	};
	static auto hook = [](HINSTANCE i, HINSTANCE pi, LPSTR m, int s) { return __WinMain(winMain, i, pi, m, s); };
	*(void**)(&part3[2]) = &hook;
	RecottePluginManager::MemoryCopyAvoidingProtection(target, part3, sizeof(part3));

	// やっぱりなんかエラー出るンゴ
}


void OnAttach()
{
	static decltype(&CreateWindowExW) createWindowExW = RecottePluginManager::OverrideIATFunction<decltype(&CreateWindowExW)>("user32.dll", "CreateWindowExW", [](auto ...args) { return _CreateWindowExW(createWindowExW, args...); });

	auto pluginFiles = std::map<std::filesystem::path, void (*)(HINSTANCE)>();
	try
	{
		Hook_WinMain();

		for (auto pluginFile : std::filesystem::directory_iterator(RecottePluginManager::ResolvePluginPath()))
		{
			auto s = pluginFile.path().extension().string();
			if (pluginFile.path().extension().string() != ".dll") continue;
			if (pluginFile.path().filename().string() == "RecottePluginManager.dll") continue;
			if (pluginFile.path().filename().string() == "d3d11.dll") continue;
			if (pluginFile.is_directory()) continue;

			auto plugin = g_Plugins[pluginFile] = LoadLibraryA(pluginFile.path().string().c_str());
			if (plugin == nullptr) throw std::format(EMSG_FAILED_PLUGIN_DLL, RecottePluginManager::GetLastErrorString(), pluginFile.path().wstring());

			auto callback = (void (WINAPI*)(HINSTANCE))GetProcAddress(plugin, "OnPluginStart");
			if (callback == nullptr) throw std::format(EMSG_FAILED_PLUGIN_STARR, RecottePluginManager::GetLastErrorString(), pluginFile.path().wstring());

			pluginFiles[pluginFile] = callback;
		}
	}
	catch (std::wstring& e)
	{
		MessageBoxW(nullptr, e.c_str(), L"RecottePlugin", MB_ICONERROR);
		exit(-1145141919);
	}
	catch (std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "RecottePlugin", MB_ICONERROR);
		exit(-1145141919);
	}

#if NODEBUG
	auto alertMessage = std::format(L"以下の{}つのPluginが読み込まれます\r\n", pluginFiles.size());
	for (auto& [pluginFile, callback] : pluginFiles)
	{
		alertMessage += std::format(L"\r\n{}", pluginFile);
	}
	MessageBoxW(nullptr, alertMessage.c_str(), L"RecottePluginLoader", MB_OK);
#endif

	for (auto& [pluginFile, callback] : pluginFiles)
	{
		try
		{
			callback(hLibMine);
		}
		catch (std::wstring& e)
		{
#if !defined(NDEBUG)
			// Debug/Releaseでレイアウトが変わるっぽい
			MessageBoxW(nullptr, *((wchar_t**)&e + 1), std::format(L"RecottePlugin::{}", pluginFile.stem().wstring()).c_str(), MB_ICONERROR);
#else
			MessageBoxW(nullptr, e.c_str(), std::format(L"RecottePlugin::{}", pluginFile.stem().wstring()).c_str(), MB_ICONERROR);
#endif
			exit(-1145141919);
		}
		catch (std::exception& e)
		{
			MessageBoxA(nullptr, e.what(), std::format("RecottePlugin::{}", pluginFile.stem().string()).c_str(), MB_ICONERROR);
			exit(-1145141919);
		}
	}
}

void OnDetach()
{
	try
	{
		for (auto plugin : g_Plugins)
		{
			g_Plugins.erase(plugin.first);

			auto callback = (void (WINAPI*)(HINSTANCE))GetProcAddress(plugin.second, "OnPluginFinish");
			if (callback == nullptr) throw std::format(EMSG_FAILED_PLUGIN_FINISH, RecottePluginManager::GetLastErrorString(), plugin.first.wstring());

			FreeLibrary(plugin.second);
		}
	}
	catch (std::wstring& e)
	{
		MessageBoxW(nullptr, e.c_str(), L"RecottePlugin", MB_ICONERROR);
		throw;
	}
	catch (std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "RecottePlugin", MB_ICONERROR);
		throw;
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		hLibMine = hinstDLL;
		hLib = LoadLibraryA("C:\\Windows\\System32\\d3d11.dll");
		if (!hLib) return FALSE;

		p[0] = GetProcAddress(hLib, "D3D11CreateDeviceForD3D12");
		p[1] = GetProcAddress(hLib, "D3DKMTCloseAdapter");
		p[2] = GetProcAddress(hLib, "D3DKMTDestroyAllocation");
		p[3] = GetProcAddress(hLib, "D3DKMTDestroyContext");
		p[4] = GetProcAddress(hLib, "D3DKMTDestroyDevice");
		p[5] = GetProcAddress(hLib, "D3DKMTDestroySynchronizationObject");
		p[6] = GetProcAddress(hLib, "D3DKMTPresent");
		p[7] = GetProcAddress(hLib, "D3DKMTQueryAdapterInfo");
		p[8] = GetProcAddress(hLib, "D3DKMTSetDisplayPrivateDriverFormat");
		p[9] = GetProcAddress(hLib, "D3DKMTSignalSynchronizationObject");
		p[10] = GetProcAddress(hLib, "D3DKMTUnlock");
		p[11] = GetProcAddress(hLib, "D3DKMTWaitForSynchronizationObject");
		p[12] = GetProcAddress(hLib, "EnableFeatureLevelUpgrade");
		p[13] = GetProcAddress(hLib, "OpenAdapter10");
		p[14] = GetProcAddress(hLib, "OpenAdapter10_2");
		p[15] = GetProcAddress(hLib, "CreateDirect3D11DeviceFromDXGIDevice");
		p[16] = GetProcAddress(hLib, "CreateDirect3D11SurfaceFromDXGISurface");
		p[17] = GetProcAddress(hLib, "D3D11CoreCreateDevice");
		p[18] = GetProcAddress(hLib, "D3D11CoreCreateLayeredDevice");
		p[19] = GetProcAddress(hLib, "D3D11CoreGetLayeredDeviceSize");
		p[20] = GetProcAddress(hLib, "D3D11CoreRegisterLayers");
		p[21] = GetProcAddress(hLib, "D3D11CreateDevice");
		p[22] = GetProcAddress(hLib, "D3D11CreateDeviceAndSwapChain");
		p[23] = GetProcAddress(hLib, "D3D11On12CreateDevice");
		p[24] = GetProcAddress(hLib, "D3DKMTCreateAllocation");
		p[25] = GetProcAddress(hLib, "D3DKMTCreateContext");
		p[26] = GetProcAddress(hLib, "D3DKMTCreateDevice");
		p[27] = GetProcAddress(hLib, "D3DKMTCreateSynchronizationObject");
		p[28] = GetProcAddress(hLib, "D3DKMTEscape");
		p[29] = GetProcAddress(hLib, "D3DKMTGetContextSchedulingPriority");
		p[30] = GetProcAddress(hLib, "D3DKMTGetDeviceState");
		p[31] = GetProcAddress(hLib, "D3DKMTGetDisplayModeList");
		p[32] = GetProcAddress(hLib, "D3DKMTGetMultisampleMethodList");
		p[33] = GetProcAddress(hLib, "D3DKMTGetRuntimeData");
		p[34] = GetProcAddress(hLib, "D3DKMTGetSharedPrimaryHandle");
		p[35] = GetProcAddress(hLib, "D3DKMTLock");
		p[36] = GetProcAddress(hLib, "D3DKMTOpenAdapterFromHdc");
		p[37] = GetProcAddress(hLib, "D3DKMTOpenResource");
		p[38] = GetProcAddress(hLib, "D3DKMTQueryAllocationResidency");
		p[39] = GetProcAddress(hLib, "D3DKMTQueryResourceInfo");
		p[40] = GetProcAddress(hLib, "D3DKMTRender");
		p[41] = GetProcAddress(hLib, "D3DKMTSetAllocationPriority");
		p[42] = GetProcAddress(hLib, "D3DKMTSetContextSchedulingPriority");
		p[43] = GetProcAddress(hLib, "D3DKMTSetDisplayMode");
		p[44] = GetProcAddress(hLib, "D3DKMTSetGammaRamp");
		p[45] = GetProcAddress(hLib, "D3DKMTSetVidPnSourceOwner");
		p[46] = GetProcAddress(hLib, "D3DKMTWaitForVerticalBlankEvent");
		p[47] = GetProcAddress(hLib, "D3DPerformance_BeginEvent");
		p[48] = GetProcAddress(hLib, "D3DPerformance_EndEvent");
		p[49] = GetProcAddress(hLib, "D3DPerformance_GetStatus");
		p[50] = GetProcAddress(hLib, "D3DPerformance_SetMarker");

		OnAttach();
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		OnDetach();
		FreeLibrary(hLib);
	}
	return TRUE;
}

extern "C"
{
	FARPROC PA = NULL;
	int JMPtoAPI();
	#pragma comment(linker, "/export:D3D11CreateDeviceForD3D12=PROXY_D3D11CreateDeviceForD3D12")
	void WINAPI PROXY_D3D11CreateDeviceForD3D12() { PA = p[0]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTCloseAdapter=PROXY_D3DKMTCloseAdapter")
	void WINAPI PROXY_D3DKMTCloseAdapter() { PA = p[1]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTDestroyAllocation=PROXY_D3DKMTDestroyAllocation")
	void WINAPI PROXY_D3DKMTDestroyAllocation() { PA = p[2]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTDestroyContext=PROXY_D3DKMTDestroyContext")
	void WINAPI PROXY_D3DKMTDestroyContext() { PA = p[3]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTDestroyDevice=PROXY_D3DKMTDestroyDevice")
	void WINAPI PROXY_D3DKMTDestroyDevice() { PA = p[4]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTDestroySynchronizationObject=PROXY_D3DKMTDestroySynchronizationObject")
	void WINAPI PROXY_D3DKMTDestroySynchronizationObject() { PA = p[5]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTPresent=PROXY_D3DKMTPresent")
	void WINAPI PROXY_D3DKMTPresent() { PA = p[6]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTQueryAdapterInfo=PROXY_D3DKMTQueryAdapterInfo")
	void WINAPI PROXY_D3DKMTQueryAdapterInfo() { PA = p[7]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTSetDisplayPrivateDriverFormat=PROXY_D3DKMTSetDisplayPrivateDriverFormat")
	void WINAPI PROXY_D3DKMTSetDisplayPrivateDriverFormat() { PA = p[8]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTSignalSynchronizationObject=PROXY_D3DKMTSignalSynchronizationObject")
	void WINAPI PROXY_D3DKMTSignalSynchronizationObject() { PA = p[9]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTUnlock=PROXY_D3DKMTUnlock")
	void WINAPI PROXY_D3DKMTUnlock() { PA = p[10]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTWaitForSynchronizationObject=PROXY_D3DKMTWaitForSynchronizationObject")
	void WINAPI PROXY_D3DKMTWaitForSynchronizationObject() { PA = p[11]; JMPtoAPI(); }
	#pragma comment(linker, "/export:EnableFeatureLevelUpgrade=PROXY_EnableFeatureLevelUpgrade")
	void WINAPI PROXY_EnableFeatureLevelUpgrade() { PA = p[12]; JMPtoAPI(); }
	#pragma comment(linker, "/export:OpenAdapter10=PROXY_OpenAdapter10")
	void WINAPI PROXY_OpenAdapter10() { PA = p[13]; JMPtoAPI(); }
	#pragma comment(linker, "/export:OpenAdapter10_2=PROXY_OpenAdapter10_2")
	void WINAPI PROXY_OpenAdapter10_2() { PA = p[14]; JMPtoAPI(); }
	#pragma comment(linker, "/export:CreateDirect3D11DeviceFromDXGIDevice=PROXY_CreateDirect3D11DeviceFromDXGIDevice")
	void WINAPI PROXY_CreateDirect3D11DeviceFromDXGIDevice() { PA = p[15]; JMPtoAPI(); }
	#pragma comment(linker, "/export:CreateDirect3D11SurfaceFromDXGISurface=PROXY_CreateDirect3D11SurfaceFromDXGISurface")
	void WINAPI PROXY_CreateDirect3D11SurfaceFromDXGISurface() { PA = p[16]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3D11CoreCreateDevice=PROXY_D3D11CoreCreateDevice")
	void WINAPI PROXY_D3D11CoreCreateDevice() { PA = p[17]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3D11CoreCreateLayeredDevice=PROXY_D3D11CoreCreateLayeredDevice")
	void WINAPI PROXY_D3D11CoreCreateLayeredDevice() { PA = p[18]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3D11CoreGetLayeredDeviceSize=PROXY_D3D11CoreGetLayeredDeviceSize")
	void WINAPI PROXY_D3D11CoreGetLayeredDeviceSize() { PA = p[19]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3D11CoreRegisterLayers=PROXY_D3D11CoreRegisterLayers")
	void WINAPI PROXY_D3D11CoreRegisterLayers() { PA = p[20]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3D11CreateDevice=PROXY_D3D11CreateDevice")
	void WINAPI PROXY_D3D11CreateDevice() { PA = p[21]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3D11CreateDeviceAndSwapChain=PROXY_D3D11CreateDeviceAndSwapChain")
	void WINAPI PROXY_D3D11CreateDeviceAndSwapChain() { PA = p[22]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3D11On12CreateDevice=PROXY_D3D11On12CreateDevice")
	void WINAPI PROXY_D3D11On12CreateDevice() { PA = p[23]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTCreateAllocation=PROXY_D3DKMTCreateAllocation")
	void WINAPI PROXY_D3DKMTCreateAllocation() { PA = p[24]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTCreateContext=PROXY_D3DKMTCreateContext")
	void WINAPI PROXY_D3DKMTCreateContext() { PA = p[25]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTCreateDevice=PROXY_D3DKMTCreateDevice")
	void WINAPI PROXY_D3DKMTCreateDevice() { PA = p[26]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTCreateSynchronizationObject=PROXY_D3DKMTCreateSynchronizationObject")
	void WINAPI PROXY_D3DKMTCreateSynchronizationObject() { PA = p[27]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTEscape=PROXY_D3DKMTEscape")
	void WINAPI PROXY_D3DKMTEscape() { PA = p[28]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTGetContextSchedulingPriority=PROXY_D3DKMTGetContextSchedulingPriority")
	void WINAPI PROXY_D3DKMTGetContextSchedulingPriority() { PA = p[29]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTGetDeviceState=PROXY_D3DKMTGetDeviceState")
	void WINAPI PROXY_D3DKMTGetDeviceState() { PA = p[30]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTGetDisplayModeList=PROXY_D3DKMTGetDisplayModeList")
	void WINAPI PROXY_D3DKMTGetDisplayModeList() { PA = p[31]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTGetMultisampleMethodList=PROXY_D3DKMTGetMultisampleMethodList")
	void WINAPI PROXY_D3DKMTGetMultisampleMethodList() { PA = p[32]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTGetRuntimeData=PROXY_D3DKMTGetRuntimeData")
	void WINAPI PROXY_D3DKMTGetRuntimeData() { PA = p[33]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTGetSharedPrimaryHandle=PROXY_D3DKMTGetSharedPrimaryHandle")
	void WINAPI PROXY_D3DKMTGetSharedPrimaryHandle() { PA = p[34]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTLock=PROXY_D3DKMTLock")
	void WINAPI PROXY_D3DKMTLock() { PA = p[35]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTOpenAdapterFromHdc=PROXY_D3DKMTOpenAdapterFromHdc")
	void WINAPI PROXY_D3DKMTOpenAdapterFromHdc() { PA = p[36]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTOpenResource=PROXY_D3DKMTOpenResource")
	void WINAPI PROXY_D3DKMTOpenResource() { PA = p[37]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTQueryAllocationResidency=PROXY_D3DKMTQueryAllocationResidency")
	void WINAPI PROXY_D3DKMTQueryAllocationResidency() { PA = p[38]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTQueryResourceInfo=PROXY_D3DKMTQueryResourceInfo")
	void WINAPI PROXY_D3DKMTQueryResourceInfo() { PA = p[39]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTRender=PROXY_D3DKMTRender")
	void WINAPI PROXY_D3DKMTRender() { PA = p[40]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTSetAllocationPriority=PROXY_D3DKMTSetAllocationPriority")
	void WINAPI PROXY_D3DKMTSetAllocationPriority() { PA = p[41]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTSetContextSchedulingPriority=PROXY_D3DKMTSetContextSchedulingPriority")
	void WINAPI PROXY_D3DKMTSetContextSchedulingPriority() { PA = p[42]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTSetDisplayMode=PROXY_D3DKMTSetDisplayMode")
	void WINAPI PROXY_D3DKMTSetDisplayMode() { PA = p[43]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTSetGammaRamp=PROXY_D3DKMTSetGammaRamp")
	void WINAPI PROXY_D3DKMTSetGammaRamp() { PA = p[44]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTSetVidPnSourceOwner=PROXY_D3DKMTSetVidPnSourceOwner")
	void WINAPI PROXY_D3DKMTSetVidPnSourceOwner() { PA = p[45]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DKMTWaitForVerticalBlankEvent=PROXY_D3DKMTWaitForVerticalBlankEvent")
	void WINAPI PROXY_D3DKMTWaitForVerticalBlankEvent() { PA = p[46]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DPerformance_BeginEvent=PROXY_D3DPerformance_BeginEvent")
	void WINAPI PROXY_D3DPerformance_BeginEvent() { PA = p[47]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DPerformance_EndEvent=PROXY_D3DPerformance_EndEvent")
	void WINAPI PROXY_D3DPerformance_EndEvent() { PA = p[48]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DPerformance_GetStatus=PROXY_D3DPerformance_GetStatus")
	void WINAPI PROXY_D3DPerformance_GetStatus() { PA = p[49]; JMPtoAPI(); }
	#pragma comment(linker, "/export:D3DPerformance_SetMarker=PROXY_D3DPerformance_SetMarker")
	void WINAPI PROXY_D3DPerformance_SetMarker() { PA = p[50]; JMPtoAPI(); }
}
