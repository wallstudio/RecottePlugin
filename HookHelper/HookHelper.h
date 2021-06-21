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

	void InjectInstructions(void* injecteeAddress, void* hookFunctionPtr, int hookFuncOperandOffset, std::vector<unsigned char> machineCode);
}