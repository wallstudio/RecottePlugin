#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	MessageBoxA(NULL, "OnPluginStart", "TestPlugin", MB_OK);
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	MessageBoxA(NULL, "OnPluginFinish", "TestPlugin", MB_OK);
}
