#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <thread>
#include <vector>
#include <string>
#include <codecvt>
#include <map>
#include <fmt/format.h>


template<typename T = void>
T* Offset(void* base, SIZE_T rva)
{
	auto va = reinterpret_cast<SIZE_T>(base) + rva;
	return reinterpret_cast<T*>(va);
}

template<typename T>
T* RvaToVa(SIZE_T offset) { return Offset<T>(GetModuleHandleW(nullptr), offset); }

IMAGE_THUNK_DATA* FindImportAddress(const std::string& moduleName, const std::string& functionName)
{
	PIMAGE_DOS_HEADER pImgDosHeaders = reinterpret_cast<PIMAGE_DOS_HEADER>(GetModuleHandleW(nullptr));
	PIMAGE_NT_HEADERS pImgNTHeaders = Offset<IMAGE_NT_HEADERS>(pImgDosHeaders, pImgDosHeaders->e_lfanew);
	PIMAGE_IMPORT_DESCRIPTOR pImgImportDesc = Offset<IMAGE_IMPORT_DESCRIPTOR>(pImgDosHeaders, pImgNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	if (pImgDosHeaders->e_magic != IMAGE_DOS_SIGNATURE)
	{
		OutputDebugStringW(L"[TestPlugin] libPE Error : e_magic is no valid DOS signature\n");
		return nullptr;
	}

	OutputDebugStringA(fmt::format("[TestPlugin] FindImportAddress {0}\n", functionName).c_str());
	for (IMAGE_IMPORT_DESCRIPTOR* iid = pImgImportDesc; iid->Name != NULL; iid++)
	{
		if (0 != _stricmp(moduleName.c_str(), RvaToVa<char>(iid->Name))) continue;

		for (int funcIdx = 0; ; funcIdx++)
		{
			auto ia = RvaToVa<IMAGE_THUNK_DATA>(iid->OriginalFirstThunk)[funcIdx];
			if (ia.u1.Function == NULL) break;
			if (ia.u1.Ordinal >> (sizeof(ia.u1.Ordinal) * 8 - 1)) continue; // 匿名（序数のみ）の場合先頭ビットが1
			if (0 != _stricmp(functionName.c_str(), RvaToVa<IMAGE_IMPORT_BY_NAME>(ia.u1.Function)->Name)) continue;

			return RvaToVa<IMAGE_THUNK_DATA>(iid->FirstThunk) + funcIdx;
		}
	}
	return nullptr;
}

bool OverrideImportFunction(const std::string& moduleName, const std::string& functionName, void* overrideFunction)
{
	auto importAddress = FindImportAddress(moduleName, functionName);
	if (importAddress == nullptr)
	{
		OutputDebugStringA(fmt::format("[TestPlugin] Not found ImportAddress {0}::{1}\n", moduleName, functionName).c_str());
		return false;
	}

	DWORD oldrights, newrights = PAGE_READWRITE;
	VirtualProtect(importAddress, sizeof(LPVOID), newrights, &oldrights);
	importAddress->u1.Function = reinterpret_cast<LONGLONG>(overrideFunction);
	VirtualProtect(importAddress, sizeof(LPVOID), oldrights, &newrights);
}

template<typename TDelegate>
TDelegate LookupFunction(const std::string& moduleName, const std::string& functionName)
{
	static std::map<std::string, FARPROC> baseFunctions = std::map<std::string, FARPROC>();

	auto id = fmt::format("{0}::{1}", moduleName, functionName);
	if (!baseFunctions.contains(id))
	{
		auto module = GetModuleHandleA(moduleName.c_str());
		if (module == nullptr)
		{
			OutputDebugStringA(fmt::format("[TestPlugin] Not found module {0}\n", moduleName).c_str());
			return nullptr;
		}
		auto function = GetProcAddress(module, functionName.c_str());
		if (module == nullptr)
		{
			OutputDebugStringA(fmt::format("[TestPlugin] Not found function {0}\n", id).c_str());
			return nullptr;
		}
		baseFunctions[id] = function;
	}
	return reinterpret_cast<TDelegate>(baseFunctions[id]);
}


typedef HWND (*CreateWindowExWCallback)(
	_In_ DWORD dwExStyle,
	_In_opt_ LPCWSTR lpClassName,
	_In_opt_ LPCWSTR lpWindowName,
	_In_ DWORD dwStyle,
	_In_ int X,
	_In_ int Y,
	_In_ int nWidth,
	_In_ int nHeight,
	_In_opt_ HWND hWndParent,
	_In_opt_ HMENU hMenu,
	_In_opt_ HINSTANCE hInstance,
	_In_opt_ LPVOID lpParam);

HWND HookCreateWindowExW(
	_In_ DWORD dwExStyle,
	_In_opt_ LPCWSTR lpClassName,
	_In_opt_ LPCWSTR lpWindowName,
	_In_ DWORD dwStyle,
	_In_ int X,
	_In_ int Y,
	_In_ int nWidth,
	_In_ int nHeight,
	_In_opt_ HWND hWndParent,
	_In_opt_ HMENU hMenu,
	_In_opt_ HINSTANCE hInstance,
	_In_opt_ LPVOID lpParam)
{
	OutputDebugStringW(fmt::format(L"[TestPlugin] CreateWindowExW: {0} / {1}\n", lpClassName != nullptr ? lpClassName : L"", lpWindowName != nullptr ? lpWindowName : L"").c_str());
	auto base = LookupFunction<CreateWindowExWCallback>("user32.dll", "CreateWindowExW");
	if (base == nullptr)
	{
		return nullptr;
	}
	return base(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	OutputDebugStringW(L"[TestPlugin] OnPluginStart");
	OverrideImportFunction("user32.dll", "CreateWindowExW", HookCreateWindowExW);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	OutputDebugStringW(L"[TestPlugin] OnPluginFinish");
}
