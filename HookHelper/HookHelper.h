#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <format>


namespace RecottePluginFoundation
{
	const auto EMSG_NOT_FOUND_ADDRESS = "�㏑���Ώۂ̋@�\���������܂���\r\nRecotteStudio�{�̂̃A�b�v�f�[�g�ɂ��݊������Ȃ��Ȃ����\��������܂�";
	const auto EMSG_NOT_FOUND_APP_DIR = "RecotteStudio�{�̂��C���X�g�[�����ꂽ�t�H���_��������܂���ł��� {}";
	const auto EMSG_NOT_FOUND_PLUGIN_DIR_ENV = "���ϐ� \"RECOTTE_PLUGIN_DIR\" �Ŏw�肳�ꂽPlugin�t�H���_�̏ꏊ���������܂���\r\n{}";
	const auto EMSG_NOT_FOUND_PLUGIN_DIR_HOME = "���[�U�[�t�H���_����Plugin�t�H���_���������܂���\r\n {}";
	const auto EMSG_NOT_FOUND_PLUGIN_DIR_UNKNOWN = "���[�U�[�t�H���_���������܂���";


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
		// �A�Z���u�����x���Ō݊����̂Ȃ�RecottePluginFundation�ł��g�������̂Ńw�b�_�������K�{

		std::vector<wchar_t> buffer;
		size_t buffSize;

		// ���ϐ����[�h�ifor Dev�j
		_wgetenv_s(&buffSize, nullptr, 0, L"RECOTTE_PLUGIN_DIR");
		if (buffSize != 0)
		{
			buffer = std::vector<wchar_t>(buffSize);
			_wgetenv_s(&buffSize, buffer.data(), buffer.size(), L"RECOTTE_PLUGIN_DIR");
			auto path = std::filesystem::path(buffer.data());

			if (!std::filesystem::exists(path)) throw std::runtime_error(std::format(EMSG_NOT_FOUND_PLUGIN_DIR_ENV, path.string()).c_str());
			return path;
		}

		// User�f�B���N�g�����[�h�i�ǉ��C���X�g�[�����Ղ��悤�Ɂj
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