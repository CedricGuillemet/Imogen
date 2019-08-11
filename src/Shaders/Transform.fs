$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

uniform vec4 u_translate;
uniform vec4 u_scale;
uniform vec4 u_rotate;

void main()
{
	vec2 rs = (v_texcoord0+u_translate.xy) * u_scale.xy;   
	rs -= 0.5;
    vec2 ro = vec2(rs.x*cos(u_rotate.x) - rs.y * sin(u_rotate.x), rs.x*sin(u_rotate.x) + rs.y * cos(u_rotate.x));
	ro += 0.5;
    vec2 nuv = ro;
	vec4 tex = texture2D(Sampler0, nuv);
	gl_FragColor = tex;
}
