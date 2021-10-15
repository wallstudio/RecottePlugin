﻿#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <filesystem>
#include <format>


namespace RecottePluginManager
{
	const auto EMSG_NOT_FOUND_ADDRESS = L"上書き対象の機能が見つけられません\r\nRecotteStudio本体のアップデートにより互換性がなくなった可能性があります";
	const auto EMSG_NOT_FOUND_APP_DIR = L"RecotteStudio本体がインストールされたフォルダが見つかりませんでした {}";
	const auto EMSG_NOT_FOUND_PLUGIN_DIR_ENV = L"環境変数 \"RECOTTE_PLUGIN_DIR\" で指定されたPluginフォルダの場所が見つけられません\r\n{}";
	const auto EMSG_NOT_FOUND_PLUGIN_DIR_HOME = L"ユーザーフォルダ内にPluginフォルダが見つけられません\r\n {}";
	const auto EMSG_NOT_FOUND_PLUGIN_DIR_UNKNOWN = L"ユーザーフォルダが見つけられません";


	// (T*)((char*)base + rva)
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

	inline void MemoryCopyAvoidingProtection(void* dst, const void* src, size_t size)
	{
		DWORD oldrights, newrights = PAGE_EXECUTE_READWRITE;
		VirtualProtect(dst, size, newrights, &oldrights);
		memcpy(dst, src, size);
		VirtualProtect(dst, size, oldrights, &newrights);
	}


	// Hook utilities

	namespace Internal
	{
		const auto EMSG_NOT_DOS_DLL = L"システムライブラリがWindows用ライブラリとして認識できませんでした\r\n{0}::{1}";
		const auto EMSG_NOT_FOUND_FUNC_IN_DLL = L"上書き対象のシステムライブラリ内機能を見つけられませんでした\r\n{0}::{1}";
		const auto EMSG_NOT_FOUND_DLL = L"システムライブラリを見つけられませんでした\r\n{}";
		const auto EMSG_NOT_FOUND_IAT = L"システムライブラリ内機能を見つけられませんでした\r\n{}";

		// ApplicationExeのImageBaseからの相対アドレス（オフセット）を解決する
		template<typename T>
		T* RvaToVa(SIZE_T offset) { return Offset<T>(GetModuleHandleW(nullptr), offset); }

		inline IMAGE_THUNK_DATA* LockupMappedFunctionFromIAT(const std::string& moduleName, const std::string& functionName)
		{
			auto pImgDosHeaders = (IMAGE_DOS_HEADER*)GetModuleHandleW(nullptr);
			auto pImgNTHeaders = Offset<IMAGE_NT_HEADERS>(pImgDosHeaders, pImgDosHeaders->e_lfanew);
			auto pImgImportDesc = Offset<IMAGE_IMPORT_DESCRIPTOR>(pImgDosHeaders, pImgNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

			if (pImgDosHeaders->e_magic != IMAGE_DOS_SIGNATURE) throw std::format(EMSG_NOT_DOS_DLL, RecottePluginManager::AsciiToWide(moduleName), RecottePluginManager::AsciiToWide(functionName));

			for (auto iid = pImgImportDesc; iid->Name != NULL; iid++)
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
			throw std::format(EMSG_NOT_FOUND_FUNC_IN_DLL, RecottePluginManager::AsciiToWide(moduleName), RecottePluginManager::AsciiToWide(functionName));
		}

		inline void* OverrideIATFunction(const std::string& moduleName, const std::string& functionName, void* newFunction)
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

	template<typename TDelegate>
	extern TDelegate OverrideIATFunction(const std::string& moduleName, const std::string& functionName, TDelegate newFunction)
	{
		return (TDelegate)Internal::OverrideIATFunction(moduleName, functionName, newFunction);
	}

	std::vector<unsigned char> FakeLoaderCode(void* address);

	// 正確にはWinMainではなく、startが取れる
	// start -> __scrt_common_main_seh -> WinMain
	inline std::function<std::int64_t()> OverrideWinMain(std::int64_t(*fakeMain)())
	{
		// AddressOfEntryPointにfakeMainの相対アドレスをぶっ刺して済ませたいけど、がDWORD（32bit）なので溢れてしまう
		// また、VirtualAlloc(GetModuleHandleW(nullptr), loader.size(), MEM_COMMIT | MEM_COMMIT, PAGE_EXECUTE_READWRITE); でImageBaseの近くをとろうとしても、確保に失敗する
		// そこで、AddressOfEntryPointの差す部分を書き換え、書き戻し処理をデリゲートで返す。これは、mainが最初に1回しか呼ばれないことが大前提。

		auto pImgDosHeaders = (IMAGE_DOS_HEADER*)GetModuleHandleW(nullptr);
		auto pImgNTHeaders = Offset<IMAGE_NT_HEADERS>(pImgDosHeaders, pImgDosHeaders->e_lfanew);
		auto entryPointRelativeAddress = &pImgNTHeaders->OptionalHeader.AddressOfEntryPoint;
		auto entryPoint = (decltype(fakeMain))Internal::RvaToVa<void>(*entryPointRelativeAddress);

		auto loader = FakeLoaderCode(fakeMain); // fakeMainの呼出し命令を生成
		auto trueCode = std::vector<unsigned char>(loader.size());
		OutputDebugStringW(std::format(L"{}", (void*)entryPoint).c_str());
		memcpy(trueCode.data(), entryPoint, trueCode.size());
		MemoryCopyAvoidingProtection(entryPoint, loader.data(), loader.size());
		auto trueCall = [=]()
		{
			MemoryCopyAvoidingProtection(entryPoint, trueCode.data(), trueCode.size()); // このアドレスに存在しないと、コード内の相対アドレスがずれる
			OutputDebugStringW(std::format(L"{}", (void*)entryPoint).c_str());
			return entryPoint(); // 内部状態が変わるのか知らんけど、中でFF参照がおこる・・・
		};

		return trueCall;
	}

	// Instruction code utilities

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

	inline std::vector<unsigned char> FakeLoaderCode(void* address)
	{
		static unsigned char templateCode[] =
		{
			Instruction::REX(true, false, false, false), 0xB8 + Instruction::Reg32::a, DUMMY_ADDRESS, // mov rax, 0xCCCCCCCCCCCCCCCCh
			Instruction::REX(true, false, false, false), 0xFF, Instruction::ModRM(4, Instruction::Mode::reg, Instruction::Reg32::a), // jmp rax;
			Instruction::NOP, Instruction::NOP, Instruction::NOP,
		};
		auto code = std::vector(std::begin(templateCode), std::end(templateCode));
		*(void**)(code.data() + 2) = address;
		return code;
	};


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