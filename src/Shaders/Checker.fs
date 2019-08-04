$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

void main()
{
	vec2 nuv = v_texcoord0 - vec2(0.5, 0.5);
    float res = mod(floor(nuv.x)+floor(nuv.y), 2.0);
    gl_FragColor = vec4(res, res, res, res);
}