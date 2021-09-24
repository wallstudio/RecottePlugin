#pragma once
#include "../HookHelper/HookHelper.h"


struct Vector2 { float x; float y; };

struct Rect { float x; float y; float w; float h; };

union LayerObj
{
	union Unknown
	{
		RecottePluginFoundation::Member<float(*)(), 0x60> getConstantMinHeight;
	};
	RecottePluginFoundation::Member<Unknown*, 0> unknown;

	union Window
	{
		RecottePluginFoundation::Member<HWND, 0x780> hwnd;
	};
	RecottePluginFoundation::Member<Window*, 0xE8> window;

	union Object
	{
		union RectInfo
		{
			RecottePluginFoundation::Member<double (*)(Object*), 184> getMin;
			RecottePluginFoundation::Member<double (*)(Object*), 200> getMax;
		};
		RecottePluginFoundation::Member<RectInfo*, 0> rectInfo;
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

union Timeline
{
	union Rect
	{
		RecottePluginFoundation::Member<void (*)(Timeline*, Vector2*), 176> getSize;
	};
	RecottePluginFoundation::Member<Rect*, 0> rect;
};
union LayerList
{
	RecottePluginFoundation::Member<LayerObj**, 48> leyers;
	RecottePluginFoundation::Member<std::int32_t, 56> leyerCount;
	RecottePluginFoundation::Member<double, 1864> scale;
};