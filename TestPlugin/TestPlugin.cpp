#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <thread>
#include <vector>
#include <string>


struct WindowInfo
{
	HWND Handle;
	std::string ClassName;
	std::string Text;
};

std::thread g_Thread;
std::vector<WindowInfo> g_Winsows = std::vector<WindowInfo>();

extern "C" __declspec(dllexport) void WINAPI OnPluginStart(HINSTANCE handle)
{
	MessageBoxA(NULL, "OnPluginStart", "TestPlugin", MB_OK);

	g_Thread = std::thread(
		[=]() -> void
		{
			while (true)
			{

				//MessageBoxA(NULL, "OnPluginStart", "--------", MB_OK);
				g_Winsows.clear();
				EnumWindows(
					[](HWND win, LPARAM param) -> BOOL
					{
						auto className = std::string(256, '\n');
						GetClassNameA(win, className.data(), className.size());
						auto text = std::string(256, '\n');
						GetWindowTextA(win, text.data(), text.size());
						g_Winsows.push_back({ win, className, text, });
						if (0 >= className.find("SumireFrameWindow"))
						{
							OutputDebugStringA("");
						}
						return TRUE;
					},
					reinterpret_cast<LPARAM>(handle));
				
			}
		});
}

extern "C" __declspec(dllexport) void WINAPI OnPluginFinish(HINSTANCE haneld)
{
	MessageBoxA(NULL, "OnPluginFinish", "TestPlugin", MB_OK);
}
