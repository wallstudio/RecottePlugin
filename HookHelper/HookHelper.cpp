#include <map>
#include <vector>
#include <fmt/format.h>
#include "HookHelper.h"


namespace RecottePluginFoundation
{
	IMAGE_THUNK_DATA* FindImportAddress(const std::string& moduleName, const std::string& functionName)
	{
		PIMAGE_DOS_HEADER pImgDosHeaders = reinterpret_cast<PIMAGE_DOS_HEADER>(GetModuleHandleW(nullptr));
		PIMAGE_NT_HEADERS pImgNTHeaders = Offset<IMAGE_NT_HEADERS>(pImgDosHeaders, pImgDosHeaders->e_lfanew);
		PIMAGE_IMPORT_DESCRIPTOR pImgImportDesc = Offset<IMAGE_IMPORT_DESCRIPTOR>(pImgDosHeaders, pImgNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

		if (pImgDosHeaders->e_magic != IMAGE_DOS_SIGNATURE)
		{
			OutputDebugStringW(L"[TestPlugin] libPE Error : e_magic is no valid DOS signature\n");
			return nullptr;
		}

		OutputDebugStringA(fmt::format("[TestPlugin] FindImportAddress {0}\n", functionName).c_str());
		for (IMAGE_IMPORT_DESCRIPTOR* iid = pImgImportDesc; iid->Name != NULL; iid++)
		{
			if (0 != _stricmp(moduleName.c_str(), RvaToVa<char>(iid->Name))) continue;

			for (int funcIdx = 0; ; funcIdx++)
			{
				auto ia = RvaToVa<IMAGE_THUNK_DATA>(iid->OriginalFirstThunk)[funcIdx];
				if (ia.u1.Function == NULL) break;
				if (ia.u1.Ordinal >> (sizeof(ia.u1.Ordinal) * 8 - 1)) continue; // 匿名（序数のみ）の場合先頭ビットが1
				if (0 != _stricmp(functionName.c_str(), RvaToVa<IMAGE_IMPORT_BY_NAME>(ia.u1.Function)->Name)) continue;

				return RvaToVa<IMAGE_THUNK_DATA>(iid->FirstThunk) + funcIdx;
			}
		}
		return nullptr;
	}

	bool OverrideImportFunction(const std::string& moduleName, const std::string& functionName, void* overrideFunction)
	{
		auto importAddress = FindImportAddress(moduleName, functionName);
		if (importAddress == nullptr)
		{
			OutputDebugStringA(fmt::format("[TestPlugin] Not found ImportAddress {0}::{1}\n", moduleName, functionName).c_str());
			return false;
		}

		DWORD oldrights, newrights = PAGE_READWRITE;
		VirtualProtect(importAddress, sizeof(LPVOID), newrights, &oldrights);
		importAddress->u1.Function = reinterpret_cast<LONGLONG>(overrideFunction);
		VirtualProtect(importAddress, sizeof(LPVOID), oldrights, &newrights);
	}

	FARPROC LookupFunction(const std::string& moduleName, const std::string& functionName)
	{
		static std::map<std::string, FARPROC> baseFunctions = std::map<std::string, FARPROC>();

		auto id = fmt::format("{0}::{1}", moduleName, functionName);
		if (!baseFunctions.contains(id))
		{
			auto module = GetModuleHandleA(moduleName.c_str());
			if (module == nullptr)
			{
				OutputDebugStringA(fmt::format("[TestPlugin] Not found module {0}\n", moduleName).c_str());
				return nullptr;
			}
			auto function = GetProcAddress(module, functionName.c_str());
			if (function == nullptr)
			{
				OutputDebugStringA(fmt::format("[TestPlugin] Not found function {0}\n", id).c_str());
				return nullptr;
			}
			baseFunctions[id] = function;
		}
		return baseFunctions[id];
	}

}
