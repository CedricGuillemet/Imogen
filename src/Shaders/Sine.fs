$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

vec2 u_translation;
float u_freq;
float u_angle;

void main()
{
	vec2 nuv = v_texcoord0 - vec2(0.5, 0.5) + u_translation;
	nuv = vec2(cos(u_angle), sin(u_angle)) * (nuv * u_freq * PI * 2.0);
    float r = cos(nuv.x + nuv.y) * 0.5 + 0.5;
	gl_FragColor = vec4(r, r, r, r);
}
