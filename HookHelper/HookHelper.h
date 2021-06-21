#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>


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
		auto jmpAddress = (void**)&machineCode[hookFuncOperandOffset];
		*jmpAddress = hookFunctionPtr;
		DWORD oldProtection;
		VirtualProtect(injecteeAddress, machineCode.size(), PAGE_EXECUTE_READWRITE, &oldProtection);
		memcpy(injecteeAddress, machineCode.data(), machineCode.size());
	}

	template<size_t Size>
	void InjectInstructions(void* hookFunctionPtr, int hookFuncOperandOffset, std::array<unsigned char, Size> markerSequence, std::array<unsigned char, Size> machineCode)
	{
		void* injecteeAddress = nullptr;
		unsigned char* address = nullptr;
		MEMORY_BASIC_INFORMATION info;
		HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
		while (VirtualQueryEx(handle, address, &info, sizeof(info)))
		{
			if(info.Type == MEM_IMAGE)
			{
				auto buffer = std::vector<unsigned char>(info.RegionSize);
				ReadProcessMemory(handle, address, buffer.data(), info.RegionSize, nullptr);

				for (size_t i = 0; i < buffer.size(); i++)
				{
					if (0 == memcmp(buffer.data() + i, markerSequence.data(), markerSequence.size()))
					{
						injecteeAddress = address + i;
						goto BREAK;
					}
				}
			}
			address += info.RegionSize;
		}
		return;
		
		BREAK:
		InjectInstructions(injecteeAddress, hookFunctionPtr, hookFuncOperandOffset, machineCode);
	}
}