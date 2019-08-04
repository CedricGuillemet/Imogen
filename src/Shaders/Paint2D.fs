$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

int u_size; // 1<<(size+8)

vec4 brushSample(vec2 uv, float radius)
{
	vec2 nuv = (uv) / radius + vec2(0.5, 0.5);
	float alphaMul = min(min(step(0.0, nuv.x), step(nuv.x, 1.0)), min(step(0.0, nuv.y), step(nuv.y, 1.0)));
	return texture2D(Sampler0, nuv) * alphaMul;
}

void main()
{
	vec4 res = vec4(0.0, 0.0, 0.0, 0.0);
	float brushRadius = 0.25;
	vec4 brush = brushSample(v_texcoord0-u_mouse.xy, brushRadius);
	if (u_uiPass == 1)
	{
		res = brush;
    }
	// paint pass
	if (u_mouse.z > 0.0)
	{
		res = brush;
	}
	if (u_mouse.w > 0.0)
	{
		res = brush;
		res.xyz = vec3(0.0, 0.0, 0.0);
	}
	
	gl_FragColor = vec4(res.xyz*res.w, res.w);	
}