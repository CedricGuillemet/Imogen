$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

vec2 u_translate;
vec2 u_scale;
float u_rotate;

void main()
{
	vec2 rs = (v_texcoord0+u_translate) * u_scale;   
	rs -= 0.5;
    vec2 ro = vec2(rs.x*cos(u_rotate) - rs.y * sin(u_rotate), rs.x*sin(u_rotate) + rs.y * cos(u_rotate));
	ro += 0.5;
    vec2 nuv = ro;
	vec4 tex = texture2D(Sampler0, nuv);
	gl_FragColor = tex;
}
