$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

uniform vec4 u_center;
uniform vec4 u_startAngle;
uniform vec4 u_splits;
uniform vec4 u_sym;

void main()
{
	vec2 center = vec2(0.5, 0.5);
	
	vec2 uv = v_texcoord0 - u_center.xy;
	float l = length(uv);
	
	float ng = atan2(uv.y, uv.x) + PI - u_startAngle.x;
	float modulo = (2.* PI) / (u_splits.x + 1.);
	float count = mod((ng / modulo) *2.0, 2.0);
	ng = mod(ng, modulo);
	if ((u_sym.x > 0.001) && (count>1.))
	{
		ng = modulo - mod(ng, modulo);
	}
	else
	{
		ng = mod(ng, modulo);
	}
	ng += u_startAngle.x;
	vec2 uv2 = vec2(-cos(-ng), sin(-ng)) * l + u_center.xy;
	vec4 tex = texture2D(Sampler0, uv2);
	gl_FragColor = tex;
}
