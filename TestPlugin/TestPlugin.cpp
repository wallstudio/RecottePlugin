#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <thread>
#include <vector>
#include <string>
#include <codecvt>

struct WindowInfo
{
	HWND Handle;
	std::string ClassName;
	std::string Text;
};

std::thread g_Thread;
std::vector<WindowInfo> g_Winsows = std::vector<WindowInfo>();


template<typename T = void>
T* Offset(void* base, SIZE_T rva)
{
	auto va = reinterpret_cast<SIZE_T>(base) + rva;
	return reinterpret_cast<T*>(va);
}

template<typename T>
T* RvaToVa(SIZE_T offset) { return Offset<T>(GetModuleHandleW(nullptr), offset); }

IMAGE_THUNK_DATA* IATfind(const std::string& function)
{
	PIMAGE_DOS_HEADER pImgDosHeaders = reinterpret_cast<PIMAGE_DOS_HEADER>(GetModuleHandleW(nullptr));
	PIMAGE_NT_HEADERS pImgNTHeaders = Offset<IMAGE_NT_HEADERS>(pImgDosHeaders, pImgDosHeaders->e_lfanew);
	PIMAGE_IMPORT_DESCRIPTOR pImgImportDesc = Offset<IMAGE_IMPORT_DESCRIPTOR>(pImgDosHeaders, pImgNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	if (pImgDosHeaders->e_magic != IMAGE_DOS_SIGNATURE)
		OutputDebugStringW(L"libPE Error : e_magic is no valid DOS signature");

	for (IMAGE_IMPORT_DESCRIPTOR* iid = pImgImportDesc; iid->Name != NULL; iid++) {
		auto dllName = RvaToVa<char>(iid->Name);
		OutputDebugStringA(dllName);
		OutputDebugStringA("\n");
		for (int funcIdx = 0; ; funcIdx++) {
			auto ia = RvaToVa<IMAGE_THUNK_DATA>(iid->OriginalFirstThunk)[funcIdx];

			if (ia.u1.Function == NULL) break;
			if (ia.u1.Ordinal >> (sizeof(ia.u1.Ordinal) * 8 - 1)) continue; // 序数のみ
			OutputDebugStringA("\t");
			OutputDebugStringA(RvaToVa<IMAGE_IMPORT_BY_NAME>(ia.u1.Function)->Name);
			OutputDebugStringA("\n");
			if (0 != _stricmp(function.c_str(), RvaToVa<IMAGE_IMPORT_BY_NAME>(ia.u1.Function)->Name)) continue;
			return RvaToVa<IMAGE_THUNK_DATA>(iid->FirstThunk) + funcIdx;
		}
	}
	return nullptr;
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
	static CreateWindowExWCallback base = nullptr;
	if (base == nullptr)
	{
		base = reinterpret_cast<CreateWindowExWCallback>(GetProcAddress(GetModuleHandle(L"user32"), "CreateWindowExW"));
	}
	return base(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	MessageBoxA(NULL, "OnPluginStart", "TestPlugin", MB_OK);

	auto function = IATfind("CreateWindowExW");
	DWORD oldrights, newrights = PAGE_READWRITE;
	VirtualProtect(function, sizeof(LPVOID), newrights, &oldrights);
	function->u1.Function = reinterpret_cast<LONGLONG>(HookCreateWindowExW);
	VirtualProtect(function, sizeof(LPVOID), oldrights, &newrights);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	MessageBoxA(NULL, "OnPluginFinish", "TestPlugin", MB_OK);
}
