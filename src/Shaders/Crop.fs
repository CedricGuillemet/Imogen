$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

vec4 u_quad;

void main()
{
	if (u_uiPass == 1)
	{
		vec4 q = vec4(min(u_quad.x, u_quad.z), 
			min(u_quad.y, u_quad.w),
			max(u_quad.x, u_quad.z), 
			max(u_quad.y, u_quad.w));
		float barx = min(step(q.x, v_texcoord0.x), step(v_texcoord0.x, q.z));
		float bary = min(step(q.y, v_texcoord0.y), step(v_texcoord0.y, q.w));
		float colFactor = min(barx, bary);
		gl_FragColor = texture2D(Sampler0, v_texcoord0) * max(colFactor, 0.5);
		return;
	}
	
	vec4 q = u_quad;
	vec2 uv = vec2(mix(min(q.x, q.z), max(q.x, q.z), v_texcoord0.x), mix(min(q.y, q.w), max(q.y, q.w), v_texcoord0.y));
	gl_FragColor = texture2D(Sampler0, uv);
}