$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

uniform vec4 u_color0;
uniform vec4 u_color1;

void main()
{
    gl_FragColor = texture2D(Sampler0, v_texcoord0) * u_color0 + u_color1;
}
