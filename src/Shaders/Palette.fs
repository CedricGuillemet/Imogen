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

#define best(X) {\
	vec4 best = pal[0];\
	for (int i = 1;i<X;i++)\
	{\
		if (length(pal[i] - original) < length(best - original))\
			best = pal[i];\
	}\
	return best;\
}

vec4 GetCGA(vec4 original, int index)
{
	vec4 pal[4];
	pal[0] = vec4(0.,0.,0.,1.);
	pal[1] = vec4(0.,0.,0.,1.);
	pal[2] = vec4(0.,0.,0.,1.);
	pal[3] = vec4(0.,0.,0.,1.);
	if (index == 0)
	{
		pal[1] = vec4(0.,0.66,0.66,1.);//0x00AAAA;
		pal[2] = vec4(0.66,0.,0.66,1.);//0xAA00AA;
		pal[3] = vec4(0.66,0.66,0.66,1.);//0xAAAAAA;
	}
	else if (index == 1)
	{
		pal[1] = vec4(0.,0.66,0.66,1.);//0x00AAAA;
		pal[2] = vec4(0.66,0.,0.,1.);//0xAA0000;
		pal[3] = vec4(0.66,0.66,0.66,1.);//0xAAAAAA;
	}
	else if (index == 2)
	{
		pal[1] = vec4(0.,0.66,0.,1.);//0x00AA00;
		pal[2] = vec4(0.66,0.,0.,1.);//0xAA0000;
		pal[3] = vec4(0.66,0.33,0.,1.);//0xAA5500;
	}
	else if (index == 3)
	{
		pal[1] = vec4(0.33,1.,1.,1.);//0x55FFFF;
		pal[2] = vec4(1.,0.33,1.,1.);//0xFF55FF;
		pal[3] = vec4(1.,1.,1.,1.);//0xFFFFFF;
	}
	else if (index == 4)
	{
		pal[1] = vec4(0.33,1.,1.,1.);//0x55FFFF;
		pal[2] = vec4(1.,0.33,0.33,1.);//0xFF5555;
		pal[3] = vec4(1.,1.,1.,1.);//0xFFFFFF;
	}
	else  if (index == 5)
	{ 
		pal[1] = vec4(0.33,1.,0.33,1.);//0x55FF55;
		pal[2] = vec4(1.,0.33,0.33,1.);//0xFF5555;
		pal[3] = vec4(1.,1.,0.33,1.);//0xFFFF55;
	}

	best(4)
}

vec4 GetEGA(vec4 original)
{
	vec4 pal[16];
	pal[0] = vec4(0.,0.,0.,1.);//0x000000;
	pal[1] = vec4(0.,0.,0.66,1.);//0x0000AA;
	pal[2] = vec4(0.,0.66,0.,1.);//0x00AA00;
	pal[3] = vec4(0.,0.66,0.66,1.);//0x00AAAA;
	pal[4] = vec4(0.66,0.,0.,1.);//0xAA0000;
	pal[5] = vec4(0.66,0.,0.66,1.);//0xAA00AA;
	pal[6] = vec4(0.66,0.33,0.,1.);//0xAA5500;
	pal[7] = vec4(0.66,0.66,0.66,1.);//0xAAAAAA;
	pal[8] = vec4(0.33,0.33,0.33,1.);//0x555555;
	pal[9] = vec4(0.33,0.33,1.,1.);//0x5555FF;
	pal[10] = vec4(0.33,1.,0.33,1.);//0x55FF55;
	pal[11] = vec4(0.33,1.,1.,1.);//0x55FFFF;
	pal[12] = vec4(1.,0.33,0.33,1.);//0xFF5555;
	pal[13] = vec4(1.,0.33,1.,1.);//0xFF55FF;
	pal[14] = vec4(1.,1.,0.33,1.);//0xFFFF55;
	pal[15] = vec4(1.,1.,1.,1.);//0xFFFFFF;

	best(16)
}

vec4 GetGameBoy(vec4 original)
{
	vec4 pal[4];
	pal[0] = vec4(0.06, 0.22, 0.06, 1.);//0x0f380f;
	pal[1] = vec4(0.19, 0.38, 0.19, 1.);//0x306230;
	pal[2] = vec4(0.54, 0.67, 0.06, 1.);//0x8bac0f;
	pal[3] = vec4(0.60, 0.74, 0.06, 1.);//0x9bbc0f;
	
	best(4)
}

vec4 GetPico8(vec4 original)
{
	vec4 pal[16];
	pal[0]=vec4(0.000, 0.000, 0.000,1.);
	pal[1]=vec4(0.373, 0.341, 0.310,1.);
	pal[2]=vec4(0.761, 0.765, 0.780,1.);
	pal[3]=vec4(1.000, 0.945, 0.910,1.);
	pal[4]=vec4(1.000, 0.925, 0.153,1.);
	pal[5]=vec4(1.000, 0.639, 0.000,1.);
	pal[6]=vec4(1.000, 0.800, 0.667,1.);
	pal[7]=vec4(0.671, 0.322, 0.212,1.);
	pal[8]=vec4(1.000, 0.467, 0.659,1.);
	pal[9]=vec4(1.000, 0.000, 0.302,1.);
	pal[10]=vec4(0.514, 0.463, 0.612,1.);
	pal[11]=vec4(0.494, 0.145, 0.325,1.);
	pal[12]=vec4(0.161, 0.678, 0.867,1.);
	pal[13]=vec4(0.114, 0.169, 0.325,1.);
	pal[14]=vec4(0.000, 0.529, 0.318,1.);
	pal[15]=vec4(0.000, 0.894, 0.212,1.);

	best(16)
}

vec4 GetC64(vec4 original)
{
	vec4 pal[16];
	pal[0]=vec4(0.000, 0.000, 0.000,1.);
	pal[1]=vec4(1.000, 1.000, 1.000,1.);
	pal[2]=vec4(0.533, 0.000, 0.000,1.);
	pal[3]=vec4(0.667, 1.000, 0.933,1.);
	pal[4]=vec4(0.800, 0.267, 0.800,1.);
	pal[5]=vec4(0.000, 0.800, 0.333,1.);
	pal[6]=vec4(0.000, 0.000, 0.667,1.);
	pal[7]=vec4(0.933, 0.933, 0.467,1.);
	pal[8]=vec4(0.867, 0.533, 0.333,1.);
	pal[9]=vec4(0.400, 0.267, 0.000,1.);
	pal[10]=vec4(1.000, 0.467, 0.467,1.);
	pal[11]=vec4(0.200, 0.200, 0.200,1.);
	pal[12]=vec4(0.467, 0.467, 0.467,1.);
	pal[13]=vec4(0.667, 1.000, 0.400,1.);
	pal[14]=vec4(0.000, 0.533, 1.000,1.);
	pal[15]=vec4(0.733, 0.733, 0.733,1.);

	best(16)
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
	if (paletteIndex == 6)
		res = GetEGA(color);
	if (paletteIndex == 7)
		res = GetGameBoy(color);
	if (paletteIndex == 8)
		res = GetPico8(color);
	if (paletteIndex == 9)
		res = GetC64(color);
	gl_FragColor = res;
}
