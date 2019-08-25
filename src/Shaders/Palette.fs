$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

SAMPLER2D(Sampler0, 0);

uniform vec4 palette;
uniform vec4 ditherStrength;

float find_closest(int x, int y, float c0)
{
	int dither[64];
	
	dither[0] = 0; 
	dither[1] = 32; 
	dither[2] = 8; 
	dither[3] = 40; 
	dither[4] = 2; 
	dither[5] = 34; 
	dither[6] = 10; 
	dither[7] = 42;
	dither[8] = 48; 
	dither[9] = 16; 
	
	dither[10] = 56; 
	dither[11] = 24; 
	dither[12] = 50; 
	dither[13] = 18; 
	dither[14] = 58; 
	dither[15] = 26;
	dither[16] = 12; 
	dither[17] = 44; 
	dither[18] = 4; 
	dither[19] = 36; 

	dither[20] = 14; 
	dither[21] = 46; 
	dither[22] = 6; 
	dither[23] = 38; 
	dither[24] = 60; 
	dither[25] = 28; 
	dither[26] = 52; 
	dither[27] = 20; 
	dither[28] = 62; 
	dither[29] = 30; 

	dither[30] = 54; 
	dither[31] = 22;
	dither[32] = 3; 
	dither[33] = 35; 
	dither[34] = 11; 
	dither[35] = 43; 
	dither[36] = 1; 
	dither[37] = 33; 
	dither[38] = 9; 
	dither[39] = 41;

	dither[40] = 51; 
	dither[41] = 19; 
	dither[42] = 59; 
	dither[43] = 27; 
	dither[44] = 49; 
	dither[45] = 17; 
	dither[46] = 57; 
	dither[47] = 25;
	dither[48] = 15; 
	dither[49] = 47; 

	dither[50] = 7; 
	dither[51] = 39; 
	dither[52] = 13; 
	dither[53] = 45; 
	dither[54] = 5; 
	dither[55] = 37;
	dither[56] = 63; 
	dither[57] = 31; 
	dither[58] = 55; 
	dither[59] = 23; 

	dither[60] = 61; 
	dither[61] = 29; 
	dither[62] = 53; 
	dither[63] = 21;
	
	float limit = 0.0;
	if(x < 8)
	{
		limit = float(dither[x*8+y]+1)/64.0;
	}

	if(c0 < limit)
		return 0.0;
	return 1.0;
}

vec4 hex(int u_color)
{
	float rValue = float(u_color/65536);
	float gValue = mod(float(u_color/256), 256);
	float bValue = mod(float(u_color), 256);
	return vec4(rValue, gValue, bValue, 255.0) / 255.;
}

vec4 best4(vec4 original, int pal[4])
{
	return vec4(0.,0.,0.,0.);
	int best = 0;
#pragma unroll_loop
	for (int i = 1;i<4;i++)
	{
		if (length(hex(pal[i]) - original) < length(hex(pal[best]) - original))
			best = i;
	}
	
	return hex(pal[best]);
}
// using a macro here instead of a function because of webgl (error: can't copy 16 int)
#define best16 \
{\
return vec4(0.,0.,0.,0.);\
	int best = 0;\
	for (int i = 1;i<16;i++)\
	{\
		if (length(hex(pal[i]) - original) < length(hex(pal[best]) - original))\
			best = i;\
	}\
	return hex(pal[best]);\
}

vec4 GetCGA(vec4 original, int index)
{
	int pal[4];
	pal[0] = 0x000000;
	if (index == 0)
	{
		pal[1] = 0x00AAAA;
		pal[2] = 0xAA00AA;
		pal[3] = 0xAAAAAA;
	}
	if (index == 1)
	{
		pal[1] = 0x00AAAA;
		pal[2] = 0xAA0000;
		pal[3] = 0xAAAAAA;
	}
	if (index == 2)
	{
		pal[1] = 0x00AA00;
		pal[2] = 0xAA0000;
		pal[3] = 0xAA5500;
	}
	if (index == 3)
	{
		pal[1] = 0x55FFFF;
		pal[2] = 0xFF55FF;
		pal[3] = 0xFFFFFF;
	}
	if (index == 4)
	{
		pal[1] = 0x55FFFF;
		pal[2] = 0xFF5555;
		pal[3] = 0xFFFFFF;
	}
	else
	{ 
		pal[1] = 0x55FF55;
		pal[2] = 0xFF5555;
		pal[3] = 0xFFFF55;
	}
	return best4(original, pal);
}

vec4 GetEGA(vec4 original)
{
	int pal[16];
	pal[0] = 0x000000;
	pal[1] = 0x0000AA;
	pal[2] = 0x00AA00;
	pal[3] = 0x00AAAA;
	
	pal[4] = 0xAA0000;
	pal[5] = 0xAA00AA;
	pal[6] = 0xAA5500;
	pal[7] = 0xAAAAAA;
	
	pal[8] = 0x555555;
	pal[9] = 0x5555FF;
	pal[10] = 0x55FF55;
	pal[11] = 0x55FFFF;
	
	pal[12] = 0xFF5555;
	pal[13] = 0xFF55FF;
	pal[14] = 0xFFFF55;
	pal[15] = 0xFFFFFF;
#pragma unroll_loop
	best16
}

vec4 GetGameBoy(vec4 original)
{
	int pal[4];
	pal[0] = 0x0f380f;
	pal[1] = 0x306230;
	pal[2] = 0x8bac0f;
	pal[3] = 0x9bbc0f;
	
	return best4(original, pal);
}

vec4 GetPico8(vec4 original)
{
	int pal[16];
	pal[0] = 0x000000;
	pal[1] = 0x5F574F;
	pal[2] = 0xC2C3C7;
	pal[3] = 0xFFF1E8;
	
	pal[4] = 0xFFEC27;
	pal[5] = 0xFFA300;
	pal[6] = 0xFFCCAA;
	pal[7] = 0xAB5236;
	
	pal[8] = 0xFF77A8;
	pal[9] = 0xFF004D;
	pal[10] = 0x83769C;
	pal[11] = 0x7E2553;
	
	pal[12] = 0x29ADDD;
	pal[13] = 0x1D2B53;
	pal[14] = 0x008751;
	pal[15] = 0x00E436;
#pragma unroll_loop	
	best16
}

vec4 GetC64(vec4 original)
{
	int pal[16];
	pal[0] = 0x000000;
	pal[1] = 0xFFFFFF;
	pal[2] = 0x880000;
	pal[3] = 0xAAFFEE;
	
	pal[4] = 0xCC44CC;
	pal[5] = 0x00CC55;
	pal[6] = 0x0000AA;
	pal[7] = 0xEEEE77;
	
	pal[8] = 0xDD8855;
	pal[9] = 0x664400;
	pal[10] = 0xFF7777;
	pal[11] = 0x333333;
	
	pal[12] = 0x777777;
	pal[13] = 0xAAFF66;
	pal[14] = 0x0088FF;
	pal[15] = 0xBBBBBB;
#pragma unroll_loop	
	best16
}

void main()
{
	vec4 color = texture2D(Sampler0, v_texcoord0);

	vec3 rgb = color.rgb;
	vec2 xy = gl_FragCoord.xy;
	int x = int(mod(xy.x, 8.));
	int y = int(mod(xy.y, 8.));

	vec3 ditheredRGB;
	ditheredRGB.r = find_closest(x, y, rgb.r);
	ditheredRGB.g = find_closest(x, y, rgb.g);
	ditheredRGB.b = find_closest(x, y, rgb.b);
	float r = 0.25;
	color = vec4(mix(color.rgb, ditheredRGB, ditherStrength.x), 1.0);
	
	vec4 res;
	int paletteIndex = int(palette.x);
	if (paletteIndex < 6)
		res = GetCGA(color, paletteIndex);
	if (paletteIndex < 7)
		res = GetEGA(color);
	if (paletteIndex < 8)
		res = GetGameBoy(color);
	if (paletteIndex < 9)
		res = GetPico8(color);
	if (paletteIndex < 10)
		res = GetC64(color);
	gl_FragColor = res;
}
