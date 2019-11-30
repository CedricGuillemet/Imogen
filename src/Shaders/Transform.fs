$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

SAMPLER2D(Sampler0, 0);

uniform vec4 translate;
uniform vec4 scale;
uniform vec4 rotate;

void main()
{
	vec2 rs = (v_texcoord0+vec2(translate.x, 1.0 - translate.y)) * scale.xy;   
	rs -= 0.5;
    vec2 ro = vec2(rs.x*cos(rotate.x) - rs.y * sin(rotate.x), rs.x*sin(rotate.x) + rs.y * cos(rotate.x));
	ro += 0.5;
    vec2 nuv = ro;
	vec4 tex = texture2D(Sampler0, nuv);
	gl_FragColor = tex;
}
