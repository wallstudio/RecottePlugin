#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <format>


namespace RecottePluginFoundation
{
	template<typename T = void>
	T* Offset(void* base, SIZE_T rva)
	{
		auto va = reinterpret_cast<SIZE_T>(base) + rva;
		return reinterpret_cast<T*>(va);
	}


	// IAT utilities

	IMAGE_THUNK_DATA* LockupMappedFunctionFromIAT(const std::string& moduleName, const std::string& functionName);
	FARPROC LookupFunctionDirect(const std::string& moduleName, const std::string& functionName);
	extern bool OverrideIATFunction(const std::string& moduleName, const std::string& functionName, void* overrideFunction);

	template<typename TDelegate>
	extern TDelegate LookupFunctionDirect(const std::string& moduleName, const std::string& functionName)
	{
		return reinterpret_cast<TDelegate>(LookupFunctionDirect(moduleName, functionName));
	}


	// Instruction code utilities

	template<size_t Size>
	void InjectInstructions(void* hookFunctionPtr, int hookFuncOperandOffset, std::array<unsigned char, Size> markerSequence, std::array<unsigned char, Size> machineCode)
	{
		void* injecteeAddress = SearchAddress(markerSequence);
		InjectInstructions(injecteeAddress, hookFunctionPtr, hookFuncOperandOffset, machineCode);
	}

	inline std::byte* SearchAddress(std::function<bool(std::byte*)> predicate)
	{
		std::byte* address = nullptr;
		MEMORY_BASIC_INFORMATION info;
		HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
		while (VirtualQueryEx(handle, address, &info, sizeof(info)))
		{
			if (info.Type == MEM_IMAGE)
			{
				//auto buffer = std::vector<std::byte>(info.RegionSize);
				//ReadProcessMemory(handle, address, buffer.data(), info.RegionSize, nullptr);

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
		return nullptr;
	}

	inline void MemoryCopyAvoidingProtection(void* dst, void* src, size_t size)
	{
		DWORD oldProtection;
		VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtection);
		memcpy(dst, src, size);
	}


	// Directory utilities

	inline std::filesystem::path ResolveApplicationDir()
	{
		auto exePathBuffer = std::vector<wchar_t>(_MAX_PATH);
		GetModuleFileNameW(GetModuleHandleW(NULL), exePathBuffer.data(), exePathBuffer.size());
		auto path = std::filesystem::path(exePathBuffer.data()).parent_path();

		if (!std::filesystem::exists(path)) throw std::runtime_error(std::format("invalid path {} (app)", path.string()).c_str());
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

			if (!std::filesystem::exists(path)) throw std::runtime_error(std::format("invalid path {} (env)", path.string()).c_str());
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

			if (!std::filesystem::exists(path)) throw std::runtime_error(std::format("invalid path {} (home)", path.string()).c_str());
			return path;
		}

		throw std::runtime_error(std::format("invalid path unknown").c_str());
	}
}