$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

float u_radius; 
float u_t; 

void main()
{
	float c = Circle(v_texcoord0, u_radius, u_t);
    gl_FragColor = vec4(c, c, c, c);
}
