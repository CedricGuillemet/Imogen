$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

int u_mode;
int u_size;

vec4 EquirectToCubemap(vec2 vuv)
{
	vec3 dir = mul(vec4(vuv * 2.0 - 1.0, 1.0, 0.0), u_viewRot).xyz;
	vec2 uv = envMapEquirect(normalize(dir));
	vec4 tex = texture2D(Sampler0, vec2(uv.x, 1.0-uv.y));
	return tex;
}

vec4 CubemapToEquirect(vec2 vuv)
{
	vec2 uv = vuv * 2.0 - 1.0;
	vec2 ng = uv * PI * vec2(1.0, 0.5);
	vec2 a = cos(ng);
	vec2 b = sin(ng);
	return textureCube(CubeSampler0, normalize(vec3(a.x*a.y, -b.y, b.x*a.y))); 
}

void main()
{
    vec4 res;
	if (u_mode == 0)
		res = EquirectToCubemap(v_texcoord0);
	else
		res = CubemapToEquirect(v_texcoord0);
    res.a = 1.0;
    gl_FragColor = res;
}
