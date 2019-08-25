$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

SAMPLERCUBE(CubeSampler0, 0);

void main() 
{
	vec2 uv = (v_texcoord0 - 0.5) * 2.0;
	vec2 ng = uv * vec2(3.14159265, 1.57079633);
	vec2 a = cos(ng);
	vec2 b = sin(ng);
	gl_FragColor = textureCube(CubeSampler0, normalize(vec3(a.x*a.y, -b.y, b.x*a.y))); 
    gl_FragColor.a = 1.0;
}
