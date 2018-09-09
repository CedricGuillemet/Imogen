#define PI 3.14159265359
#define TwoPI (PI*2)

#ifdef VERTEX_SHADER

layout(location = 0)in vec2 inUV;
out vec2 vUV;

void main()
{
    gl_Position = vec4(inUV.xy*2.0-1.0,0.5,1.0); 
	vUV = inUV;
}

#endif


#ifdef FRAGMENT_SHADER

layout(location=0) out vec4 outPixDiffuse;
in vec2 vUV;

uniform sampler2D Sampler0;
uniform sampler2D Sampler1;
uniform sampler2D Sampler2;
uniform sampler2D Sampler3;
uniform sampler2D equiRectEnvSampler;

#include "Nodes.glsl"
#include "Previews.glsl"

void main() 
{ 
	outPixDiffuse = vec4(__FUNCTION__);
	outPixDiffuse.a = 1.0;
}

#endif
