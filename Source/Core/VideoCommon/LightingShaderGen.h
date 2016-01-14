// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include "Common/Assert.h"
#include "Common/CommonTypes.h"
#include "VideoCommon/NativeVertexFormat.h"
#include "VideoCommon/ShaderGenCommon.h"
#include "VideoCommon/XFMemory.h"


#define LIGHT_COL "%s[%d].color.%s"
#define LIGHT_COL_PARAMS(index, swizzle) (I_LIGHTS), (index), (swizzle)

#define LIGHT_COSATT "%s[%d].cosatt"
#define LIGHT_COSATT_PARAMS(index) (I_LIGHTS), (index)

#define LIGHT_DISTATT "%s[%d].distatt"
#define LIGHT_DISTATT_PARAMS(index) (I_LIGHTS), (index)

#define LIGHT_POS "%s[%d].pos"
#define LIGHT_POS_PARAMS(index) (I_LIGHTS), (index)

#define LIGHT_DIR "%s[%d].dir"
#define LIGHT_DIR_PARAMS(index) (I_LIGHTS), (index)

/**
 * Common uid data used for shader generators that use lighting calculations.
 */
struct LightingUidData
{
	u32 matsource      : 4;  // 4x1 bit
	u32 enablelighting : 4;  // 4x1 bit
	u32 ambsource      : 4;  // 4x1 bit
	u32 diffusefunc    : 8;  // 4x2 bits
	u32 attnfunc       : 8;  // 4x2 bits
	u32 light_mask     : 32; // 4x8 bits
};

static const char s_lighting_struct[] =
	"struct Light {\n"
	"\tint4 color;\n"
	"\tfloat4 cosatt;\n"
	"\tfloat4 distatt;\n"
	"\tfloat4 pos;\n"
	"\tfloat4 dir;\n"
	"};\n";

template<class T>
static void GenerateLightShader(T& object, LightingUidData& uid_data, int index, int litchan_index, bool alpha)
{
	const char* swizzle = alpha ? "a" : "rgb";
	const char* swizzle_components = (alpha) ? "" : "3";

	int attnfunc = (uid_data.attnfunc >> (2 * litchan_index)) & 0x3;
	int diffusefunc = (uid_data.diffusefunc >> (2 * litchan_index)) & 0x3;

	switch (attnfunc)
	{
		case LIGHTATTN_NONE:
		case LIGHTATTN_DIR:
			object.Write("ldir = normalize(" LIGHT_POS".xyz - pos.xyz);\n", LIGHT_POS_PARAMS(index));
			object.Write("attn = 1.0;\n");
			object.Write("if (length(ldir) == 0.0)\n\t ldir = _norm0;\n");
			break;
		case LIGHTATTN_SPEC:
			object.Write("ldir = normalize(" LIGHT_POS".xyz - pos.xyz);\n", LIGHT_POS_PARAMS(index));
			object.Write("attn = (dot(_norm0, ldir) >= 0.0) ? max(0.0, dot(_norm0, " LIGHT_DIR".xyz)) : 0.0;\n", LIGHT_DIR_PARAMS(index));
			object.Write("cosAttn = " LIGHT_COSATT".xyz;\n", LIGHT_COSATT_PARAMS(index));
			object.Write("distAttn = %s(" LIGHT_DISTATT".xyz);\n", (diffusefunc == LIGHTDIF_NONE) ? "" : "normalize", LIGHT_DISTATT_PARAMS(index));
			object.Write("attn = max(0.0f, dot(cosAttn, float3(1.0, attn, attn*attn))) / dot(distAttn, float3(1.0, attn, attn*attn));\n");
			break;
		case LIGHTATTN_SPOT:
			object.Write("ldir = " LIGHT_POS".xyz - pos.xyz;\n", LIGHT_POS_PARAMS(index));
			object.Write("dist2 = dot(ldir, ldir);\n"
			             "dist = sqrt(dist2);\n"
			             "ldir = ldir / dist;\n"
			             "attn = max(0.0, dot(ldir, " LIGHT_DIR".xyz));\n", LIGHT_DIR_PARAMS(index));
			// attn*attn may overflow
			object.Write("attn = max(0.0, " LIGHT_COSATT".x + " LIGHT_COSATT".y*attn + " LIGHT_COSATT".z*attn*attn) / dot(" LIGHT_DISTATT".xyz, float3(1.0,dist,dist2));\n",
			             LIGHT_COSATT_PARAMS(index), LIGHT_COSATT_PARAMS(index), LIGHT_COSATT_PARAMS(index), LIGHT_DISTATT_PARAMS(index));
			break;
	}

	switch (diffusefunc)
	{
		case LIGHTDIF_NONE:
			object.Write("lacc.%s += int%s(round(attn * float%s(" LIGHT_COL")));\n",
			             swizzle, swizzle_components,
			             swizzle_components, LIGHT_COL_PARAMS(index, swizzle));
			break;
		case LIGHTDIF_SIGN:
		case LIGHTDIF_CLAMP:
			object.Write("lacc.%s += int%s(round(attn * %sdot(ldir, _norm0)) * float%s(" LIGHT_COL")));\n",
			             swizzle, swizzle_components,
			             diffusefunc != LIGHTDIF_SIGN ? "max(0.0," :"(",
			             swizzle_components, LIGHT_COL_PARAMS(index, swizzle));
			break;
		default: _assert_(0);
	}

	object.Write("\n");
}

// vertex shader
// lights/colors
// materials name is I_MATERIALS in vs and I_PMATERIALS in ps
// inColorName is color in vs and colors_ in ps
// dest is o.colors_ in vs and colors_ in ps
template<class T>
static void GenerateLightingShader(T& object, LightingUidData& uid_data, int components, const char* inColorName, const char* dest)
{
	for (unsigned int j = 0; j < xfmem.numChan.numColorChans; j++)
	{
		const LitChannel& color = xfmem.color[j];
		const LitChannel& alpha = xfmem.alpha[j];

		object.Write("{\n");

		uid_data.matsource |= xfmem.color[j].matsource << j;
		bool colormatsource = !!(uid_data.matsource & (1 << j));
		if (colormatsource) // from vertex
		{
			if (components & (VB_HAS_COL0 << j))
				object.Write("int4 mat = int4(round(%s%d * 255.0));\n", inColorName, j);
			else if (components & VB_HAS_COL0)
				object.Write("int4 mat = int4(round(%s0 * 255.0));\n", inColorName);
			else
				object.Write("int4 mat = int4(255, 255, 255, 255);\n");
		}
		else // from color
		{
			object.Write("int4 mat = %s[%d];\n", I_MATERIALS, j+2);
		}

		uid_data.enablelighting |= xfmem.color[j].enablelighting << j;
		if (uid_data.enablelighting & (1 << j))
		{
			uid_data.ambsource |= xfmem.color[j].ambsource << j;
			if (uid_data.ambsource & (1 << j)) // from vertex
			{
				if (components & (VB_HAS_COL0<<j) )
					object.Write("lacc = int4(round(%s%d * 255.0));\n", inColorName, j);
				else if (components & VB_HAS_COL0 )
					object.Write("lacc = int4(round(%s0 * 255.0));\n", inColorName);
				else
					// TODO: this isn't verified. Here we want to read the ambient from the vertex,
					// but the vertex itself has no color. So we don't know which value to read.
					// Returning 1.0 is the same as disabled lightning, so this could be fine
					object.Write("lacc = int4(255, 255, 255, 255);\n");
			}
			else // from color
			{
				object.Write("lacc = %s[%d];\n", I_MATERIALS, j);
			}
		}
		else
		{
			object.Write("lacc = int4(255, 255, 255, 255);\n");
		}

		// check if alpha is different
		uid_data.matsource |= xfmem.alpha[j].matsource << (j+2);
		bool alphamatsource = !!(uid_data.matsource & (1 << (j+2)));
		if (alphamatsource != colormatsource)
		{
			if (alphamatsource) // from vertex
			{
				if (components & (VB_HAS_COL0<<j))
					object.Write("mat.w = int(round(%s%d.w * 255.0));\n", inColorName, j);
				else if (components & VB_HAS_COL0)
					object.Write("mat.w = int(round(%s0.w * 255.0));\n", inColorName);
				else object.Write("mat.w = 255;\n");
			}
			else // from color
			{
				object.Write("mat.w = %s[%d].w;\n", I_MATERIALS, j+2);
			}
		}

		uid_data.enablelighting |= xfmem.alpha[j].enablelighting << (j+2);
		if (uid_data.enablelighting & (1 << (j+2)))
		{
			uid_data.ambsource |= xfmem.alpha[j].ambsource << (j+2);
			if (uid_data.ambsource & (1 << (j+2))) // from vertex
			{
				if (components & (VB_HAS_COL0<<j) )
					object.Write("lacc.w = int(round(%s%d.w * 255.0));\n", inColorName, j);
				else if (components & VB_HAS_COL0 )
					object.Write("lacc.w = int(round(%s0.w * 255.0));\n", inColorName);
				else
					// TODO: The same for alpha: We want to read from vertex, but the vertex has no color
					object.Write("lacc.w = 255;\n");
			}
			else // from color
			{
				object.Write("lacc.w = %s[%d].w;\n", I_MATERIALS, j);
			}
		}
		else
		{
			object.Write("lacc.w = 255;\n");
		}

		if (uid_data.enablelighting & (1 << j)) // Color lights
		{
			uid_data.attnfunc |= color.attnfunc << (2 * j);
			uid_data.diffusefunc |= color.diffusefunc << (2 * j);
			uid_data.light_mask |= color.GetFullLightMask() << (8 * j);
			for (int i = 0; i < 8; ++i)
				if (uid_data.light_mask & (1 << (i + 8 * j)))
					GenerateLightShader<T>(object, uid_data, i, j, false);
		}
		if (uid_data.enablelighting & (1 << (j+2))) // Alpha lights
		{
			uid_data.attnfunc |= alpha.attnfunc << (2 * (j+2));
			uid_data.diffusefunc |= alpha.diffusefunc << (2 * (j+2));
			uid_data.light_mask |= alpha.GetFullLightMask() << (8 * (j+2));
			for (int i = 0; i < 8; ++i)
				if (uid_data.light_mask & (1 << (i + 8 * (j+2))))
					GenerateLightShader<T>(object, uid_data, i, j+2, true);
		}
		object.Write("lacc = clamp(lacc, 0, 255);\n");
		object.Write("%s%d = float4((mat * (lacc + (lacc >> 7))) >> 8) / 255.0;\n", dest, j);
		object.Write("}\n");
	}
}
