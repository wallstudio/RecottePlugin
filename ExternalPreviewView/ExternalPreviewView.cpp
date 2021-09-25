#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "../HookHelper/HookHelper.h"


extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	auto c = RecottePluginFoundation::Intenal::OverrideIATFunction;
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld) {}
