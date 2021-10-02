#pragma once
#include "HookHelper.h"


struct Vector2 { float x; float y; };

struct Rect { float x; float y; float w; float h; };

// FIXME: 1.4.7.0 アップデートで構造が変化しているので、この定義は間違っている
union LayerObj
{
	RecottePluginManager::VirtualMember<float(*)(), 0x60, 0> getConstantMinHeight;

	union Window
	{
		RecottePluginManager::Member<HWND, 0x780> hwnd;
	};
	RecottePluginManager::Member<Window*, 0xE8> window;

	union Object
	{
		RecottePluginManager::VirtualMember<double (*)(Object*), 184, 0> getMin;
		RecottePluginManager::VirtualMember<double (*)(Object*), 200, 0> getMax;
		RecottePluginManager::Member<LayerObj*, 296> layerInfo;
		RecottePluginManager::Member<float, 784> y;
		RecottePluginManager::Member<float, 792> h;
	};
	RecottePluginManager::Member<Object**, 0x130> objects;

	RecottePluginManager::Member<std::int32_t, 0x138> objectCount;
	RecottePluginManager::Member<float, 0x160> layerHeight;
	RecottePluginManager::Member<bool, 0x379> invalid;
	RecottePluginManager::Member<float, 892> leyerMinY;
};

union A1
{
	union Timeline
	{
		RecottePluginManager::VirtualMember<void (*)(Timeline*, Vector2*), 176, 0> getSize;
	};
	RecottePluginManager::Member<Timeline*, 3176> timeline;

	union Unknown0
	{
		union LayerList
		{
			RecottePluginManager::Member<LayerObj**, 48> leyers;
			RecottePluginManager::Member<std::int32_t, 56> leyerCount;
			RecottePluginManager::Member<double, 1864> scale;
		};
		RecottePluginManager::Member<LayerList*, 2960> layerList;
	};
	RecottePluginManager::Member<Unknown0*, 2768> unknown0;

	union Unknown1
	{
		RecottePluginManager::Member<double, 2640> offset;
	};
	RecottePluginManager::Member<Unknown1*, 3200> unknown1;
};