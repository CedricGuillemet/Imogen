$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

uniform vec4 radius; 
uniform vec4 T; 

void main()
{
	float c = Circle(v_texcoord0, radius.x, T.x);
    gl_FragColor = vec4(c, c, c, 1.0);
}
