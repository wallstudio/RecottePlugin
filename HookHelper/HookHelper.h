#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <format>


namespace RecottePluginFoundation
{
	const auto EMSG_NOT_FOUND_ADDRESS = "上書き対象の機能が見つけられません\r\nRecotteStudio本体のアップデートにより互換性がなくなった可能性があります";
	const auto EMSG_NOT_FOUND_APP_DIR = "RecotteStudio本体がインストールされたフォルダが見つかりませんでした {}";
	const auto EMSG_NOT_FOUND_PLUGIN_DIR_ENV = "環境変数 \"RECOTTE_PLUGIN_DIR\" で指定されたPluginフォルダの場所が見つけられません\r\n{}";
	const auto EMSG_NOT_FOUND_PLUGIN_DIR_HOME = "ユーザーフォルダ内にPluginフォルダが見つけられません\r\n {}";
	const auto EMSG_NOT_FOUND_PLUGIN_DIR_UNKNOWN = "ユーザーフォルダが見つけられません";


	template<typename T = void>
	T* Offset(void* base, SIZE_T rva)
	{
		auto va = reinterpret_cast<SIZE_T>(base) + rva;
		return reinterpret_cast<T*>(va);
	}


	// IAT utilities

	IMAGE_THUNK_DATA* LockupMappedFunctionFromIAT(const std::string& moduleName, const std::string& functionName);
	FARPROC LookupFunctionFromWin32Api(const std::string& moduleName, const std::string& functionName);
	extern void OverrideIATFunction(const std::string& moduleName, const std::string& functionName, void* overrideFunction);

	template<typename TDelegate>
	extern TDelegate LookupFunctionFromWin32Api(const std::string& moduleName, const std::string& functionName)
	{
		return reinterpret_cast<TDelegate>(LookupFunctionFromWin32Api(moduleName, functionName));
	}


	// Instruction code utilities

	inline std::byte* SearchAddress(std::function<bool(std::byte*)> predicate)
	{
		std::byte* address = nullptr;
		MEMORY_BASIC_INFORMATION info;
		HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
		while (VirtualQueryEx(handle, address, &info, sizeof(info)))
		{
			if (info.Type == MEM_IMAGE)
			{
				for (size_t i = 0; i < info.RegionSize; i++)
				{
					if (predicate(address + i))
					{
						return address + i;
					}
				}
			}
			address += info.RegionSize;
		}
		throw std::runtime_error(std::format(EMSG_NOT_FOUND_ADDRESS).c_str());
	}

	inline void MemoryCopyAvoidingProtection(void* dst, void* src, size_t size)
	{
		DWORD oldrights, newrights = PAGE_EXECUTE_READWRITE;
		VirtualProtect(dst, size, newrights, &oldrights);
		memcpy(dst, src, size);
		VirtualProtect(dst, size, oldrights, &newrights);
	}


	// Directory utilities

	inline std::filesystem::path ResolveApplicationDir()
	{
		auto exePathBuffer = std::vector<wchar_t>(_MAX_PATH);
		GetModuleFileNameW(GetModuleHandleW(NULL), exePathBuffer.data(), exePathBuffer.size());
		auto path = std::filesystem::path(exePathBuffer.data()).parent_path();

		if (!std::filesystem::exists(path)) throw std::runtime_error(std::format(EMSG_NOT_FOUND_APP_DIR, path.string()).c_str());
		return path;
	}

	inline std::filesystem::path ResolvePluginPath()
	{
		// アセンブリレベルで互換性のないRecottePluginFundationでも使いたいのでヘッダ実装が必須

		std::vector<wchar_t> buffer;
		size_t buffSize;

		// 環境変数モード（for Dev）
		_wgetenv_s(&buffSize, nullptr, 0, L"RECOTTE_PLUGIN_DIR");
		if (buffSize != 0)
		{
			buffer = std::vector<wchar_t>(buffSize);
			_wgetenv_s(&buffSize, buffer.data(), buffer.size(), L"RECOTTE_PLUGIN_DIR");
			auto path = std::filesystem::path(buffer.data());

			if (!std::filesystem::exists(path)) throw std::runtime_error(std::format(EMSG_NOT_FOUND_PLUGIN_DIR_ENV, path.string()).c_str());
			return path;
		}

		// Userディレクトリモード（追加インストールし易いように）
		_wgetenv_s(&buffSize, nullptr, 0, L"HOMEPATH");
		buffer = std::vector<wchar_t>(buffSize);
		_wgetenv_s(&buffSize, buffer.data(), buffer.size(), L"HOMEPATH");
		if (buffSize != 0)
		{
			auto userDir = std::format(L"C:{}", buffer.data());
			auto path = std::filesystem::path(userDir) / "RecottePlugin";

			if (!std::filesystem::exists(path)) throw std::runtime_error(std::format(EMSG_NOT_FOUND_PLUGIN_DIR_HOME, path.string()).c_str());
			return path;
		}

		throw std::runtime_error(std::format(EMSG_NOT_FOUND_PLUGIN_DIR_UNKNOWN).c_str());
	}
}