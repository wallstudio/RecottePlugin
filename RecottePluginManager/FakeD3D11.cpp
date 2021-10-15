#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <format>


const auto EMSG_FAIL_LOAD_D3D11 = L"�V�X�e���ɃC���X�g�[�����ꂽd3d11.dll���������܂���ł���\n{}";

FARPROC p[51] = { 0 };

HINSTANCE LoadTrueLibrary()
{
	auto path = L"C:\\Windows\\System32\\d3d11.dll";
	auto trueLib = LoadLibraryW(path);
	if (!trueLib) throw std::format(EMSG_FAIL_LOAD_D3D11, path);

	p[0] = GetProcAddress(trueLib, "D3D11CreateDeviceForD3D12");
	p[1] = GetProcAddress(trueLib, "D3DKMTCloseAdapter");
	p[2] = GetProcAddress(trueLib, "D3DKMTDestroyAllocation");
	p[3] = GetProcAddress(trueLib, "D3DKMTDestroyContext");
	p[4] = GetProcAddress(trueLib, "D3DKMTDestroyDevice");
	p[5] = GetProcAddress(trueLib, "D3DKMTDestroySynchronizationObject");
	p[6] = GetProcAddress(trueLib, "D3DKMTPresent");
	p[7] = GetProcAddress(trueLib, "D3DKMTQueryAdapterInfo");
	p[8] = GetProcAddress(trueLib, "D3DKMTSetDisplayPrivateDriverFormat");
	p[9] = GetProcAddress(trueLib, "D3DKMTSignalSynchronizationObject");
	p[10] = GetProcAddress(trueLib, "D3DKMTUnlock");
	p[11] = GetProcAddress(trueLib, "D3DKMTWaitForSynchronizationObject");
	p[12] = GetProcAddress(trueLib, "EnableFeatureLevelUpgrade");
	p[13] = GetProcAddress(trueLib, "OpenAdapter10");
	p[14] = GetProcAddress(trueLib, "OpenAdapter10_2");
	p[15] = GetProcAddress(trueLib, "CreateDirect3D11DeviceFromDXGIDevice");
	p[16] = GetProcAddress(trueLib, "CreateDirect3D11SurfaceFromDXGISurface");
	p[17] = GetProcAddress(trueLib, "D3D11CoreCreateDevice");
	p[18] = GetProcAddress(trueLib, "D3D11CoreCreateLayeredDevice");
	p[19] = GetProcAddress(trueLib, "D3D11CoreGetLayeredDeviceSize");
	p[20] = GetProcAddress(trueLib, "D3D11CoreRegisterLayers");
	p[21] = GetProcAddress(trueLib, "D3D11CreateDevice");
	p[22] = GetProcAddress(trueLib, "D3D11CreateDeviceAndSwapChain");
	p[23] = GetProcAddress(trueLib, "D3D11On12CreateDevice");
	p[24] = GetProcAddress(trueLib, "D3DKMTCreateAllocation");
	p[25] = GetProcAddress(trueLib, "D3DKMTCreateContext");
	p[26] = GetProcAddress(trueLib, "D3DKMTCreateDevice");
	p[27] = GetProcAddress(trueLib, "D3DKMTCreateSynchronizationObject");
	p[28] = GetProcAddress(trueLib, "D3DKMTEscape");
	p[29] = GetProcAddress(trueLib, "D3DKMTGetContextSchedulingPriority");
	p[30] = GetProcAddress(trueLib, "D3DKMTGetDeviceState");
	p[31] = GetProcAddress(trueLib, "D3DKMTGetDisplayModeList");
	p[32] = GetProcAddress(trueLib, "D3DKMTGetMultisampleMethodList");
	p[33] = GetProcAddress(trueLib, "D3DKMTGetRuntimeData");
	p[34] = GetProcAddress(trueLib, "D3DKMTGetSharedPrimaryHandle");
	p[35] = GetProcAddress(trueLib, "D3DKMTLock");
	p[36] = GetProcAddress(trueLib, "D3DKMTOpenAdapterFromHdc");
	p[37] = GetProcAddress(trueLib, "D3DKMTOpenResource");
	p[38] = GetProcAddress(trueLib, "D3DKMTQueryAllocationResidency");
	p[39] = GetProcAddress(trueLib, "D3DKMTQueryResourceInfo");
	p[40] = GetProcAddress(trueLib, "D3DKMTRender");
	p[41] = GetProcAddress(trueLib, "D3DKMTSetAllocationPriority");
	p[42] = GetProcAddress(trueLib, "D3DKMTSetContextSchedulingPriority");
	p[43] = GetProcAddress(trueLib, "D3DKMTSetDisplayMode");
	p[44] = GetProcAddress(trueLib, "D3DKMTSetGammaRamp");
	p[45] = GetProcAddress(trueLib, "D3DKMTSetVidPnSourceOwner");
	p[46] = GetProcAddress(trueLib, "D3DKMTWaitForVerticalBlankEvent");
	p[47] = GetProcAddress(trueLib, "D3DPerformance_BeginEvent");
	p[48] = GetProcAddress(trueLib, "D3DPerformance_EndEvent");
	p[49] = GetProcAddress(trueLib, "D3DPerformance_GetStatus");
	p[50] = GetProcAddress(trueLib, "D3DPerformance_SetMarker");
	return trueLib;
}

extern "C"
{
	FARPROC PA = NULL;
	int JMPtoAPI();
#pragma comment(linker, "/export:D3D11CreateDeviceForD3D12=PROXY_D3D11CreateDeviceForD3D12")
	void WINAPI PROXY_D3D11CreateDeviceForD3D12() { PA = p[0]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTCloseAdapter=PROXY_D3DKMTCloseAdapter")
	void WINAPI PROXY_D3DKMTCloseAdapter() { PA = p[1]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTDestroyAllocation=PROXY_D3DKMTDestroyAllocation")
	void WINAPI PROXY_D3DKMTDestroyAllocation() { PA = p[2]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTDestroyContext=PROXY_D3DKMTDestroyContext")
	void WINAPI PROXY_D3DKMTDestroyContext() { PA = p[3]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTDestroyDevice=PROXY_D3DKMTDestroyDevice")
	void WINAPI PROXY_D3DKMTDestroyDevice() { PA = p[4]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTDestroySynchronizationObject=PROXY_D3DKMTDestroySynchronizationObject")
	void WINAPI PROXY_D3DKMTDestroySynchronizationObject() { PA = p[5]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTPresent=PROXY_D3DKMTPresent")
	void WINAPI PROXY_D3DKMTPresent() { PA = p[6]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTQueryAdapterInfo=PROXY_D3DKMTQueryAdapterInfo")
	void WINAPI PROXY_D3DKMTQueryAdapterInfo() { PA = p[7]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTSetDisplayPrivateDriverFormat=PROXY_D3DKMTSetDisplayPrivateDriverFormat")
	void WINAPI PROXY_D3DKMTSetDisplayPrivateDriverFormat() { PA = p[8]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTSignalSynchronizationObject=PROXY_D3DKMTSignalSynchronizationObject")
	void WINAPI PROXY_D3DKMTSignalSynchronizationObject() { PA = p[9]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTUnlock=PROXY_D3DKMTUnlock")
	void WINAPI PROXY_D3DKMTUnlock() { PA = p[10]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTWaitForSynchronizationObject=PROXY_D3DKMTWaitForSynchronizationObject")
	void WINAPI PROXY_D3DKMTWaitForSynchronizationObject() { PA = p[11]; JMPtoAPI(); }
#pragma comment(linker, "/export:EnableFeatureLevelUpgrade=PROXY_EnableFeatureLevelUpgrade")
	void WINAPI PROXY_EnableFeatureLevelUpgrade() { PA = p[12]; JMPtoAPI(); }
#pragma comment(linker, "/export:OpenAdapter10=PROXY_OpenAdapter10")
	void WINAPI PROXY_OpenAdapter10() { PA = p[13]; JMPtoAPI(); }
#pragma comment(linker, "/export:OpenAdapter10_2=PROXY_OpenAdapter10_2")
	void WINAPI PROXY_OpenAdapter10_2() { PA = p[14]; JMPtoAPI(); }
#pragma comment(linker, "/export:CreateDirect3D11DeviceFromDXGIDevice=PROXY_CreateDirect3D11DeviceFromDXGIDevice")
	void WINAPI PROXY_CreateDirect3D11DeviceFromDXGIDevice() { PA = p[15]; JMPtoAPI(); }
#pragma comment(linker, "/export:CreateDirect3D11SurfaceFromDXGISurface=PROXY_CreateDirect3D11SurfaceFromDXGISurface")
	void WINAPI PROXY_CreateDirect3D11SurfaceFromDXGISurface() { PA = p[16]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3D11CoreCreateDevice=PROXY_D3D11CoreCreateDevice")
	void WINAPI PROXY_D3D11CoreCreateDevice() { PA = p[17]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3D11CoreCreateLayeredDevice=PROXY_D3D11CoreCreateLayeredDevice")
	void WINAPI PROXY_D3D11CoreCreateLayeredDevice() { PA = p[18]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3D11CoreGetLayeredDeviceSize=PROXY_D3D11CoreGetLayeredDeviceSize")
	void WINAPI PROXY_D3D11CoreGetLayeredDeviceSize() { PA = p[19]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3D11CoreRegisterLayers=PROXY_D3D11CoreRegisterLayers")
	void WINAPI PROXY_D3D11CoreRegisterLayers() { PA = p[20]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3D11CreateDevice=PROXY_D3D11CreateDevice")
	void WINAPI PROXY_D3D11CreateDevice() { PA = p[21]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3D11CreateDeviceAndSwapChain=PROXY_D3D11CreateDeviceAndSwapChain")
	void WINAPI PROXY_D3D11CreateDeviceAndSwapChain() { PA = p[22]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3D11On12CreateDevice=PROXY_D3D11On12CreateDevice")
	void WINAPI PROXY_D3D11On12CreateDevice() { PA = p[23]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTCreateAllocation=PROXY_D3DKMTCreateAllocation")
	void WINAPI PROXY_D3DKMTCreateAllocation() { PA = p[24]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTCreateContext=PROXY_D3DKMTCreateContext")
	void WINAPI PROXY_D3DKMTCreateContext() { PA = p[25]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTCreateDevice=PROXY_D3DKMTCreateDevice")
	void WINAPI PROXY_D3DKMTCreateDevice() { PA = p[26]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTCreateSynchronizationObject=PROXY_D3DKMTCreateSynchronizationObject")
	void WINAPI PROXY_D3DKMTCreateSynchronizationObject() { PA = p[27]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTEscape=PROXY_D3DKMTEscape")
	void WINAPI PROXY_D3DKMTEscape() { PA = p[28]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTGetContextSchedulingPriority=PROXY_D3DKMTGetContextSchedulingPriority")
	void WINAPI PROXY_D3DKMTGetContextSchedulingPriority() { PA = p[29]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTGetDeviceState=PROXY_D3DKMTGetDeviceState")
	void WINAPI PROXY_D3DKMTGetDeviceState() { PA = p[30]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTGetDisplayModeList=PROXY_D3DKMTGetDisplayModeList")
	void WINAPI PROXY_D3DKMTGetDisplayModeList() { PA = p[31]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTGetMultisampleMethodList=PROXY_D3DKMTGetMultisampleMethodList")
	void WINAPI PROXY_D3DKMTGetMultisampleMethodList() { PA = p[32]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTGetRuntimeData=PROXY_D3DKMTGetRuntimeData")
	void WINAPI PROXY_D3DKMTGetRuntimeData() { PA = p[33]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTGetSharedPrimaryHandle=PROXY_D3DKMTGetSharedPrimaryHandle")
	void WINAPI PROXY_D3DKMTGetSharedPrimaryHandle() { PA = p[34]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTLock=PROXY_D3DKMTLock")
	void WINAPI PROXY_D3DKMTLock() { PA = p[35]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTOpenAdapterFromHdc=PROXY_D3DKMTOpenAdapterFromHdc")
	void WINAPI PROXY_D3DKMTOpenAdapterFromHdc() { PA = p[36]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTOpenResource=PROXY_D3DKMTOpenResource")
	void WINAPI PROXY_D3DKMTOpenResource() { PA = p[37]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTQueryAllocationResidency=PROXY_D3DKMTQueryAllocationResidency")
	void WINAPI PROXY_D3DKMTQueryAllocationResidency() { PA = p[38]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTQueryResourceInfo=PROXY_D3DKMTQueryResourceInfo")
	void WINAPI PROXY_D3DKMTQueryResourceInfo() { PA = p[39]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTRender=PROXY_D3DKMTRender")
	void WINAPI PROXY_D3DKMTRender() { PA = p[40]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTSetAllocationPriority=PROXY_D3DKMTSetAllocationPriority")
	void WINAPI PROXY_D3DKMTSetAllocationPriority() { PA = p[41]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTSetContextSchedulingPriority=PROXY_D3DKMTSetContextSchedulingPriority")
	void WINAPI PROXY_D3DKMTSetContextSchedulingPriority() { PA = p[42]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTSetDisplayMode=PROXY_D3DKMTSetDisplayMode")
	void WINAPI PROXY_D3DKMTSetDisplayMode() { PA = p[43]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTSetGammaRamp=PROXY_D3DKMTSetGammaRamp")
	void WINAPI PROXY_D3DKMTSetGammaRamp() { PA = p[44]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTSetVidPnSourceOwner=PROXY_D3DKMTSetVidPnSourceOwner")
	void WINAPI PROXY_D3DKMTSetVidPnSourceOwner() { PA = p[45]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DKMTWaitForVerticalBlankEvent=PROXY_D3DKMTWaitForVerticalBlankEvent")
	void WINAPI PROXY_D3DKMTWaitForVerticalBlankEvent() { PA = p[46]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DPerformance_BeginEvent=PROXY_D3DPerformance_BeginEvent")
	void WINAPI PROXY_D3DPerformance_BeginEvent() { PA = p[47]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DPerformance_EndEvent=PROXY_D3DPerformance_EndEvent")
	void WINAPI PROXY_D3DPerformance_EndEvent() { PA = p[48]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DPerformance_GetStatus=PROXY_D3DPerformance_GetStatus")
	void WINAPI PROXY_D3DPerformance_GetStatus() { PA = p[49]; JMPtoAPI(); }
#pragma comment(linker, "/export:D3DPerformance_SetMarker=PROXY_D3DPerformance_SetMarker")
	void WINAPI PROXY_D3DPerformance_SetMarker() { PA = p[50]; JMPtoAPI(); }
}