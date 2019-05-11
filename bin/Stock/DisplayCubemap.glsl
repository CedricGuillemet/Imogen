#ifdef VERTEX_SHADER

layout(location = 0)in vec2 inUV;
out vec2 vUV;
void main()
{ 
	gl_Position = vec4(inUV.xy*2.0 - 1.0,0.5,1.0);
	vUV = inUV; 
}

#endif

#ifdef FRAGMENT_SHADER

uniform samplerCube samplerCubemap;
layout(location = 0) out vec4 outPixDiffuse;
in vec2 vUV;

void main() 
{
	vec2 uv = (vUV - 0.5) * 2.0;
	vec2 ng = uv * vec2(3.14159265, 1.57079633);
	vec2 a = cos(ng);
	vec2 b = sin(ng);
	outPixDiffuse = texture(samplerCubemap, normalize(vec3(a.x*a.y, -b.y, b.x*a.y))); 
    outPixDiffuse.a = 1.0;
}

#endif
