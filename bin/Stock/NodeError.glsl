#ifdef VERTEX_SHADER

layout(location = 0)in vec2 inUV;
out vec2 vUV;

void main()
{ 
	gl_Position = vec4(inUV.xy*2.0 - 1.0,0.5,1.0); vUV = inUV; 
}

#endif

#ifdef FRAGMENT_SHADER

uniform float time;
#define PI 3.1415926
layout(location = 0) out vec4 outPixDiffuse;
in vec2 vUV;

void main() 
{
	// base of shader by FabriceNeyret2 https://www.shadertoy.com/view/XlfBW7
	vec2 U = vUV*2.5-1.25;
    U.x = asin( U.x / cos( U.y = asin(U.y) )) - time; 
    U = 6.* sin(8.*U);       
	outPixDiffuse = vec4(0.5,0.0,0.0,1.0);
    outPixDiffuse.r = max(outPixDiffuse.r, outPixDiffuse.r+U.x/U.x);
    outPixDiffuse.gb += U.x*U.y;
}

#endif
