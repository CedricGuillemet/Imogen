$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

vec2 u_center;
float u_startAngle;
int u_splits;
int u_sym;

void main()
{
	vec2 center = vec2(0.5, 0.5);
	
	vec2 uv = v_texcoord0 - u_center;
	float l = length(uv);
	
	float ng = atan2(uv.y, uv.x) + PI - u_startAngle;
	float modulo = (2.* PI) / float(u_splits + 1);
	float count = mod((ng / modulo) *2.0, 2.0);
	ng = mod(ng, modulo);
	if ((u_sym != 0) && (count>1.))
	{
		ng = modulo - mod(ng, modulo);
	}
	else
	{
		ng = mod(ng, modulo);
	}
	ng += u_startAngle;
	vec2 uv2 = vec2(-cos(-ng), sin(-ng)) * l + u_center;
	vec4 tex = texture2D(Sampler0, uv2);
	gl_FragColor = tex;
}
