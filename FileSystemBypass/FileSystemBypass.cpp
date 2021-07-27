#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	OutputDebugStringW(L"[FileSystemBypass] " __FUNCTION__ L"\n");
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	OutputDebugStringW(L"[FileSystemBypass]" __FUNCTION__ L"\n");
}
