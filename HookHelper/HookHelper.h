#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <format>

#if !defined(HOOK_HELPER) && !defined(RecottePluginManager_EXPORTS)
#ifdef _DEBUG
#pragma comment(lib, "../x64/Debug/HookHelper.lib")
#else
#pragma comment(lib, "../x64/Release/HookHelper.lib")
#endif
#endif


namespace RecottePluginManager
{
	const auto EMSG_NOT_FOUND_ADDRESS = L"上書き対象の機能が見つけられません\r\nRecotteStudio本体のアップデートにより互換性がなくなった可能性があります";
	const auto EMSG_NOT_FOUND_APP_DIR = L"RecotteStudio本体がインストールされたフォルダが見つかりませんでした {}";
	const auto EMSG_NOT_FOUND_PLUGIN_DIR_ENV = L"環境変数 \"RECOTTE_PLUGIN_DIR\" で指定されたPluginフォルダの場所が見つけられません\r\n{}";
	const auto EMSG_NOT_FOUND_PLUGIN_DIR_HOME = L"ユーザーフォルダ内にPluginフォルダが見つけられません\r\n {}";
	const auto EMSG_NOT_FOUND_PLUGIN_DIR_UNKNOWN = L"ユーザーフォルダが見つけられません";


	template<typename T = void>
	T* Offset(void* base, SIZE_T rva)
	{
		auto va = reinterpret_cast<SIZE_T>(base) + rva;
		return reinterpret_cast<T*>(va);
	}

	inline std::wstring AsciiToWide(std::string ascii)
	{
		return std::wstring(ascii.begin(), ascii.end());
	}

	inline std::wstring GetLastErrorString()
	{
		auto code = GetLastError();
		LPTSTR buffer = nullptr;
		auto flag = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
		FormatMessageW(flag, nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buffer, 0, nullptr);
		auto str = std::wstring(buffer);
		LocalFree(buffer);
		return str;
	}


	// Hook utilities

	namespace Internal
	{
		FARPROC LookupFunctionFromWin32Api(const std::string& moduleName, const std::string& functionName);
		extern void* OverrideIATFunction(const std::string& moduleName, const std::string& functionName, void* newFunction);
	}

	template<typename TDelegate>
	extern TDelegate LookupFunctionFromWin32Api(const std::string& moduleName, const std::string& functionName)
	{
		return reinterpret_cast<TDelegate>(Internal::LookupFunctionFromWin32Api(moduleName, functionName));
	}

	template<typename TDelegate>
	extern TDelegate OverrideIATFunction(const std::string& moduleName, const std::string& functionName, TDelegate newFunction)
	{
		return (TDelegate)Internal::OverrideIATFunction(moduleName, functionName, newFunction);
	}

	namespace Instruction
	{
		enum class Mode : std::uint8_t { mem, mem_disp8, mem_disp32, reg, };
		enum class Reg32 : std::uint8_t { a, c, d, b, sp, bp, si, di, };
		enum class RegEx : std::uint8_t { _8, _9, _10, _11, _12, _13, _14, _15, };
		enum class RegXmm : std::uint8_t { _0, _1, _2, _3, _4, _5, _6, _7, };
		enum class Sp : std::uint8_t { sib = 0b100, ip = 0b101, };
		struct Reg
		{
			std::uint8_t raw;
			constexpr Reg(Reg32 _32) { raw = (std::uint8_t)_32; };
			constexpr Reg(RegEx _ex) { raw = (std::uint8_t)_ex; };
			constexpr Reg(RegXmm _xmm) { raw = (std::uint8_t)_xmm; };
			constexpr Reg(std::uint8_t opeCodeEx) { raw = opeCodeEx; };
		};
		struct RegMem
		{
			std::uint8_t raw;
			constexpr RegMem(Reg32 _32) { raw = (std::uint8_t)_32; };
			constexpr RegMem(RegEx _ex) { raw = (std::uint8_t)_ex; };
			constexpr RegMem(RegXmm _xmm) { raw = (std::uint8_t)_xmm; };
			constexpr RegMem(Sp sp) { raw = (std::uint8_t)sp; }
		};
		constexpr unsigned char operator+(unsigned char _byte, Reg32 _32) { return _byte + (unsigned char)_32; }
		constexpr unsigned char operator+(unsigned char _byte, RegEx _ex) { return _byte + (unsigned char)_ex; }
		constexpr unsigned char operator+(unsigned char _byte, RegXmm _xmm) { return _byte + (unsigned char)_xmm; }
		constexpr unsigned char operator+(unsigned char _byte, Reg reg) { return _byte + (unsigned char)reg.raw; }

		// https://www.wdic.org/w/SCI/REXプリフィックス
		constexpr unsigned char REX(bool mode64, bool shiftReg, bool shiftSibIndex, bool shiftRM)
		{
			unsigned char rex = 0;
			rex |= 0b0100 << 4;
			rex |= (mode64 ? 1 : 0) << 3;
			rex |= (shiftReg ? 1 : 0) << 2;
			rex |= (shiftSibIndex ? 1 : 0) << 1;
			rex |= (shiftRM ? 1 : 0) << 0;
			return rex;
		};

		// https://www.wdic.org/w/SCI/ModR/M
		constexpr unsigned char ModRM(Reg reg, Mode mod, RegMem regMem)
		{
			unsigned char modRM = 0;
			modRM |= (unsigned char)mod << 6;
			modRM |= (unsigned char)reg.raw << 3;
			modRM |= (unsigned char)regMem.raw << 0;
			return modRM;
		}

		// https://www.wdic.org/w/SCI/SIBバイト
		constexpr unsigned char SIB(Reg base, Reg index, std::uint8_t scale)
		{
			unsigned char sib = 0;
			sib |= scale << 6;
			sib |= (unsigned char)index.raw << 3;
			sib |= (unsigned char)base.raw << 0;
			return sib;
		}

		const unsigned char NOP = 0x90;

		#define DUMMY_ADDRESS 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC
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
		throw std::format(EMSG_NOT_FOUND_ADDRESS);
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
		GetModuleFileNameW(GetModuleHandleW(NULL), exePathBuffer.data(), (DWORD)exePathBuffer.size());
		auto path = std::filesystem::path(exePathBuffer.data()).parent_path();

		if (!std::filesystem::exists(path)) throw std::format(EMSG_NOT_FOUND_APP_DIR, path.wstring());
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

			if (!std::filesystem::exists(path)) throw std::format(EMSG_NOT_FOUND_PLUGIN_DIR_ENV, path.wstring());
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

			if (!std::filesystem::exists(path)) throw std::format(EMSG_NOT_FOUND_PLUGIN_DIR_HOME, path.wstring());
			return path;
		}

		throw std::format(EMSG_NOT_FOUND_PLUGIN_DIR_UNKNOWN);
	}


	// object utilities

	template<typename _TValue, size_t _Offset>
	class Member
	{
	private:
		std::byte dummy[_Offset];
		_TValue value;
	public:
		const _TValue& get() { return value; }
		const _TValue& operator ->() { return value; }
		_TValue operator =(_TValue v) { return (value = v); }

	};

	template<typename _TValue>
	class Member<_TValue, 0>
	{
		_TValue value;
	public:
		const _TValue& get() { return value; }
		const _TValue& operator ->() { return value; }
		_TValue operator =(_TValue v) { return (value = v); }

	};

	template<typename _TValue, size_t _MemberOffset, size_t _VTableOffset>
	class VirtualMember
	{
	private:
		std::byte dummy[_VTableOffset];
		Member<_TValue, _MemberOffset>* vTable;
	public:
		const _TValue& get() { return vTable->get(); }
		const _TValue& operator ->() { return vTable->get(); }
		_TValue operator =(_TValue v) { return (vTable->get() = v); }
	};

	template<typename _TValue, size_t _MemberOffset>
	class VirtualMember<_TValue, _MemberOffset, 0>
	{
	private:
		Member<_TValue, _MemberOffset>* vTable;
	public:
		const _TValue& get() { return vTable->get(); }
		const _TValue& operator ->() { return vTable->get(); }
		_TValue operator =(_TValue v) { return (vTable->get() = v); }
	};
}