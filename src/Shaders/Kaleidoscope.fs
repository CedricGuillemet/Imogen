$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

SAMPLER2D(Sampler0, 0);

uniform vec4 center;
uniform vec4 startAngle;
uniform vec4 splits;
uniform vec4 symetry;

void main()
{
	vec2 center = vec2(0.5, 0.5);
	
	vec2 uv = v_texcoord0 - center.xy;
	float l = length(uv);
	
	float ng = atan2(uv.y, uv.x) + PI - startAngle.x;
	float modulo = (2.* PI) / (splits.x + 1.);
	float count = mod((ng / modulo) *2.0, 2.0);
	ng = mod(ng, modulo);
	if ((symetry.x > 0.001) && (count>1.))
	{
		ng = modulo - mod(ng, modulo);
	}
	else
	{
		ng = mod(ng, modulo);
	}
	ng += startAngle.x;
	vec2 uv2 = vec2(-cos(-ng), sin(-ng)) * l + center.xy;
	vec4 tex = texture2D(Sampler0, uv2);
	gl_FragColor = tex;
}
