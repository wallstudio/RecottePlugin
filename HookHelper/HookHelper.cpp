#include <map>
#include <vector>
#include <filesystem>
#include "HookHelper.h"


namespace RecottePluginFoundation
{
	const auto EMSG_NOT_DOS_DLL = "システムライブラリがWindows用ライブラリとして認識できませんでした\r\n{0}::{1}";
	const auto EMSG_NOT_FOUND_FUNC_IN_DLL = "上書き対象のシステムライブラリ内機能を見つけられませんでした\r\n{0}::{1}";
	const auto EMSG_NOT_FOUND_DLL = "システムライブラリを見つけられませんでした\r\n{}";
	const auto EMSG_NOT_FOUND_IAT = "システムライブラリ内機能を見つけられませんでした\r\n{}";


	template<typename T>
	T* RvaToVa(SIZE_T offset) { return Offset<T>(GetModuleHandleW(nullptr), offset); }

	IMAGE_THUNK_DATA* LockupMappedFunctionFromIAT(const std::string& moduleName, const std::string& functionName)
	{
		PIMAGE_DOS_HEADER pImgDosHeaders = reinterpret_cast<PIMAGE_DOS_HEADER>(GetModuleHandleW(nullptr));
		PIMAGE_NT_HEADERS pImgNTHeaders = Offset<IMAGE_NT_HEADERS>(pImgDosHeaders, pImgDosHeaders->e_lfanew);
		PIMAGE_IMPORT_DESCRIPTOR pImgImportDesc = Offset<IMAGE_IMPORT_DESCRIPTOR>(pImgDosHeaders, pImgNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

		if (pImgDosHeaders->e_magic != IMAGE_DOS_SIGNATURE) throw std::runtime_error(std::format(EMSG_NOT_DOS_DLL, moduleName, functionName).c_str());

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
		throw std::runtime_error(std::format(EMSG_NOT_FOUND_FUNC_IN_DLL, moduleName, functionName).c_str());;
	}

	FARPROC Intenal::LookupFunctionFromWin32Api(const std::string& moduleName, const std::string& functionName)
	{
		static std::map<std::string, FARPROC> baseFunctions = std::map<std::string, FARPROC>();

		auto id = std::format("{0}::{1}", moduleName, functionName);
		if (!baseFunctions.contains(id))
		{
			auto module = GetModuleHandleA(moduleName.c_str());
			if (module == nullptr) throw std::runtime_error(std::format(EMSG_NOT_FOUND_DLL, id).c_str());;

			auto function = GetProcAddress(module, functionName.c_str());
			if (function == nullptr) throw std::runtime_error(std::format(EMSG_NOT_FOUND_IAT, id).c_str());;
			
			baseFunctions[id] = function;
		}
		return baseFunctions[id];
	}

	void* Intenal::OverrideIATFunction(const std::string& moduleName, const std::string& functionName, void* newFunction)
	{
		auto importAddress = LockupMappedFunctionFromIAT(moduleName, functionName);
		DWORD oldrights, newrights = PAGE_READWRITE;
		VirtualProtect(importAddress, sizeof(LPVOID), newrights, &oldrights);
		auto old = (void*)importAddress->u1.Function;
		importAddress->u1.Function = (LONGLONG)newFunction; // override function pointer
		VirtualProtect(importAddress, sizeof(LPVOID), oldrights, &newrights);
		return old;
	}

}
