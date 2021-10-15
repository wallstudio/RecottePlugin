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
}

void OnAttach()
{
	try
	{
		static decltype(&CreateWindowExW) createWindowExW = RecottePluginManager::OverrideIATFunction<decltype(&CreateWindowExW)>("user32.dll", "CreateWindowExW", [](auto ...args) { return _CreateWindowExW(createWindowExW, args...); });

		auto pluginFiles = std::map<std::filesystem::path, void (*)()>();
		for (auto pluginFile : std::filesystem::directory_iterator(RecottePluginManager::ResolvePluginPath()))
		{
			auto s = pluginFile.path().extension().string();
			if (pluginFile.path().extension().string() != ".dll") continue;
			if (pluginFile.path().filename().string() == "RecottePluginManager.dll") continue;
			if (pluginFile.path().filename().string() == "d3d11.dll") continue;
			if (pluginFile.is_directory()) continue;

			auto plugin = g_Plugins[pluginFile] = LoadLibraryA(pluginFile.path().string().c_str());
			if (plugin == nullptr) throw std::format(EMSG_FAILED_PLUGIN_DLL, RecottePluginManager::GetLastErrorString(), pluginFile.path().wstring());

			auto callback = (void (WINAPI*)())GetProcAddress(plugin, "OnPluginStart");
			if (callback == nullptr) throw std::format(EMSG_FAILED_PLUGIN_STARR, RecottePluginManager::GetLastErrorString(), pluginFile.path().wstring());

			pluginFiles[pluginFile] = callback;
		}

		for (auto& [pluginFile, callback] : pluginFiles)
		{
			try
			{
				callback();
			}
			catch (std::wstring& e)
			{
#if !defined(NDEBUG)
				throw std::format(L"Plugin:{}\n\n{}", pluginFile.filename().wstring(), std::wstring(*((wchar_t**)&e + 1))); // Debug/Releaseでレイアウトが変わるっぽい
#else
				throw std::format(L"Plugin:{}\n\n{}", pluginFile.filename().wstring(), e); // Debug/Releaseでレイアウトが変わるっぽい
#endif
			}
			catch (std::exception& e)
			{
				throw new std::runtime_error(std::format("Plugin:{}\n\n{}", pluginFile.filename().string(), e.what()));
			}
		}

		//LoadDotNetLibs();
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
}

void OnDetach()
{
	try
	{
		auto callbacks = std::map<std::filesystem::path, void(*)()>();
		for (auto& [pluginFile, module] : g_Plugins)
		{
			auto callback = (void (WINAPI*)())GetProcAddress(module, "OnPluginFinish");
			if (callback == nullptr) throw std::format(EMSG_FAILED_PLUGIN_FINISH, RecottePluginManager::GetLastErrorString(), pluginFile.wstring());
			callbacks[pluginFile] = callback;
		}

		for (auto& [pluginFile, callback] : callbacks)
		{
			try
			{
				callback();
		}
			catch (std::wstring& e)
			{
#if !defined(NDEBUG)
				throw std::format(L"Plugin:{}\n\n{}", pluginFile.filename().wstring(), std::wstring(*((wchar_t**)&e + 1))); // Debug/Releaseでレイアウトが変わるっぽい
#else
				throw std::format(L"Plugin:{}\n\n{}", pluginFile.filename().wstring(), e); // Debug/Releaseでレイアウトが変わるっぽい
#endif
			}
			catch (std::exception& e)
			{
				throw new std::runtime_error(std::format("Plugin:{}\n\n{}", pluginFile.filename().string(), e.what()));
			}
		}

		for (auto& [pluginFile, module] : g_Plugins)
		{
			FreeLibrary(module);
		}
		g_Plugins.clear();
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
}

void Hook_WinMain(int (*fakeMain)(decltype(&WinMain) base, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd))
{
	using namespace RecottePluginManager::Instruction;

	static HINSTANCE hInstance = nullptr;
	static decltype(&WinMain) winMain = nullptr;
	static unsigned char part[] =
	{
		0x4C, 0x8B, 0xC0,                               // mov r8, rax ; lpCmdLine
		0x44, 0x8B, 0xCB,                               // mov r9d, ebx ; nShowCmd
	};
	auto target = RecottePluginManager::SearchAddress([&](std::byte* address)
	{
		if (0 != memcmp(address, part, sizeof(part))) return false;
		address += sizeof(part);

		static unsigned char part0[] =
		{
			0x33, 0xD2,                                     // xor edx, edx ; hPrevInstance
			0x48, 0x8D, 0x0D, /* 0x1F, 0x1F, 0xBD, 0xFF, */ // lea rcx, cs:140000000h ; hInstance
		};
		if (0 != memcmp(address, part0, sizeof(part0))) return false;
		address += sizeof(part0);
		auto hInstanceOffset = *(std::int32_t*)address;
		address += sizeof(std::int32_t);
		hInstance = (HINSTANCE)RecottePluginManager::Offset<void>(address, hInstanceOffset);

		static unsigned char part1[] =
		{
			0xE8, /* 0x9A, 0xEB, 0xC2, 0xFF, */             // call WinMain
		};
		address += sizeof(part1);
		auto winMainOffset = *(std::int32_t*)address;
		address += sizeof(std::int32_t);
		winMain = RecottePluginManager::Offset<decltype(WinMain)>(address, winMainOffset);

		return true;
	});
	unsigned char part3[] =
	{
		REX(true, false, false, false), 0xB8 + Reg32::a, DUMMY_ADDRESS, // mov rax, 0FFFFFFFFFFFFFFFFh
		0xFF, ModRM(2, Mode::reg, Reg32::a), // call rax
		NOP, NOP, NOP, NOP, // nops
	};
	static auto _fakeMain = fakeMain;
	static decltype(&WinMain) hook = [](HINSTANCE _, HINSTANCE __, LPSTR m, int s) { return _fakeMain(winMain, hInstance, nullptr, m, s); };
	*(void**)(&part3[2]) = hook;
	RecottePluginManager::MemoryCopyAvoidingProtection(target + sizeof(part), part3, sizeof(part3));
}

// FakeD3D11.cpp
HINSTANCE LoadTrueLibrary();

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	try
	{
		static HMODULE trueLib = nullptr;
		if (fdwReason == DLL_PROCESS_ATTACH)
		{
			trueLib = LoadTrueLibrary();

			Hook_WinMain([](auto base, auto ...args)
			{
				OnAttach();
				auto result = base(args...);
				OnDetach();
				return result;
			});
		}
		else if (fdwReason == DLL_PROCESS_DETACH)
		{
			FreeLibrary(trueLib);
			trueLib = nullptr;
		}
		return true;
	}
	catch (std::wstring& e)
	{
		MessageBoxW(nullptr, e.c_str(), L"RecottePlugin", MB_ICONERROR);
		return false;
	}
	catch (std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "RecottePlugin", MB_ICONERROR);
		return false;
	}
}
