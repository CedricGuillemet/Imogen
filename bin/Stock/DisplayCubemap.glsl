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

/* cross
vec2 uv = fragCoord / iResolution.xy;
uv.y = 1.0-uv.y;
vec4 col = vec4(0.0,0.0,0.0,1.0);
vec2 ngs = floor(uv * vec2(4.0,3.0)+vec2(1.0,0.0)) * 3.14159 * 0.5;
vec2 cs = cos(ngs);
vec2 sn = sin(ngs);
uv.y = 1.0-uv.y;
vec3 nd=vec3(0.0,0.0,0.0);
if (uv.y>0.333 && uv.y<0.666)
{
uv.x =  -(fract(uv.x*4.0) * 2.0 - 1.0);
vec3 d = vec3(uv.x, uv.y*6.0-3.0, 1.0);
nd = vec3(d.x*cs.x - d.z*sn.x, d.y, d.x*sn.x + d.z*cs.x);
}
else
if (uv.x>0.25 && uv.x<0.5)
{
uv.y = fract(uv.y*3.0) * 2.0 - 1.0;
vec3 d = vec3(uv.x*8.0-3.0, 1.0, uv.y);
nd = vec3(d.x, d.y*cs.y - d.z*sn.y, d.y*sn.y + d.z*cs.y);
}

fragColor = texture(iChannel0, nd);
*/

void main() 
{
	vec2 uv = (vUV - 0.5) * 2.0;
	vec2 ng = uv * vec2(3.14159265, 1.57079633);
	vec2 a = cos(ng);
	vec2 b = sin(ng);
	outPixDiffuse = texture(sampler, normalize(vec3(a.x*a.y, -b.y, b.x*a.y))); 
}

#endif
