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

	template<typename T>
	T* RvaToVa(SIZE_T offset) { return Offset<T>(GetModuleHandleW(nullptr), offset); }

	IMAGE_THUNK_DATA* FindImportAddress(const std::string& moduleName, const std::string& functionName);

	extern bool OverrideImportFunction(const std::string& moduleName, const std::string& functionName, void* overrideFunction);

	FARPROC LookupFunction(const std::string& moduleName, const std::string& functionName);

	template<typename TDelegate>
	extern TDelegate LookupFunction(const std::string& moduleName, const std::string& functionName)
	{
		return reinterpret_cast<TDelegate>(LookupFunction(moduleName, functionName));
	}

	template<size_t Size>
	void InjectInstructions(void* injecteeAddress, void* hookFunctionPtr, int hookFuncOperandOffset, std::array<unsigned char, Size> machineCode)
	{
		if (injecteeAddress == nullptr)
		{
			return;
		}

		auto jmpAddress = (void**)&machineCode[hookFuncOperandOffset];
		*jmpAddress = hookFunctionPtr;
		DWORD oldProtection;
		VirtualProtect(injecteeAddress, machineCode.size(), PAGE_EXECUTE_READWRITE, &oldProtection);
		memcpy(injecteeAddress, machineCode.data(), machineCode.size());
	}

	template<size_t Size>
	unsigned char* SearchAddress(std::array<unsigned char, Size> markerSequence)
	{
		unsigned char* address = nullptr;
		MEMORY_BASIC_INFORMATION info;
		HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
		while (VirtualQueryEx(handle, address, &info, sizeof(info)))
		{
			if (info.Type == MEM_IMAGE)
			{
				auto buffer = std::vector<unsigned char>(info.RegionSize);
				ReadProcessMemory(handle, address, buffer.data(), info.RegionSize, nullptr);

				for (size_t i = 0; i < buffer.size(); i++)
				{
					if (0 == memcmp(buffer.data() + i, markerSequence.data(), markerSequence.size()))
					{
						return address + i;
					}
				}
			}
			address += info.RegionSize;
		}
		return nullptr;
	}

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
			return std::filesystem::path(buffer.data());
		}

		// Userディレクトリモード（追加インストールし易いように）
		_wgetenv_s(&buffSize, nullptr, 0, L"HOMEPATH");
		buffer = std::vector<wchar_t>(buffSize);
		_wgetenv_s(&buffSize, buffer.data(), buffer.size(), L"HOMEPATH");
		if (buffSize != 0)
		{
			auto userDir = std::format(L"C:{}", buffer.data());
			auto directory = std::filesystem::path(userDir).append("RecottePlugin");
			if (std::filesystem::exists(directory))
			{
				return directory;
			}
		}

		// 従来のProgram Filesに直接置くモード
		buffer = std::vector<wchar_t>(_MAX_PATH);
		GetModuleFileNameW(GetModuleHandleW(NULL), buffer.data(), buffer.size());
		auto pluginsDirectroy = std::filesystem::path(buffer.data()).parent_path().append("Plugins");
		return pluginsDirectroy;
	}
}