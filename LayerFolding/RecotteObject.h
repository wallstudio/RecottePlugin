#pragma once
#include "../HookHelper/HookHelper.h"


struct Vector2 { float x; float y; };

struct Rect { float x; float y; float w; float h; };

union LayerObj
{
	RecottePluginFoundation::VirtualMember<float(*)(), 0x60, 0> getConstantMinHeight;

	union Window
	{
		RecottePluginFoundation::Member<HWND, 0x780> hwnd;
	};
	RecottePluginFoundation::Member<Window*, 0xE8> window;

	union Object
	{
		RecottePluginFoundation::VirtualMember<double (*)(Object*), 184, 0> getMin;
		RecottePluginFoundation::VirtualMember<double (*)(Object*), 200, 0> getMax;
		RecottePluginFoundation::Member<LayerObj*, 296> layerInfo;
		RecottePluginFoundation::Member<float, 784> y;
		RecottePluginFoundation::Member<float, 792> h;
	};
	RecottePluginFoundation::Member<Object**, 0x130> objects;

	RecottePluginFoundation::Member<std::int32_t, 0x138> objectCount;
	RecottePluginFoundation::Member<float, 0x160> layerHeight;
	RecottePluginFoundation::Member<bool, 0x379> invalid;
	RecottePluginFoundation::Member<float, 892> leyerMinY;
};

union A1
{
	union Timeline
	{
		RecottePluginFoundation::VirtualMember<void (*)(Timeline*, Vector2*), 176, 0> getSize;
	};
	RecottePluginFoundation::Member<Timeline*, 3176> timeline;

	union Unknown0
	{
		union LayerList
		{
			RecottePluginFoundation::Member<LayerObj**, 48> leyers;
			RecottePluginFoundation::Member<std::int32_t, 56> leyerCount;
			RecottePluginFoundation::Member<double, 1864> scale;
		};
		RecottePluginFoundation::Member<LayerList*, 2960> layerList;
	};
	RecottePluginFoundation::Member<Unknown0*, 2768> unknown0;

	union Unknown1
	{
		RecottePluginFoundation::Member<double, 2640> offset;
	};
	RecottePluginFoundation::Member<Unknown1*, 3200> unknown1;
};