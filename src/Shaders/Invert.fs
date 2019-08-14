$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

void main()
{
    vec4 res = vec4(1.0,1.0,1.0,1.0) - texture2D(Sampler0, v_texcoord0);
    res.a = 1.0;
    gl_FragColor = res;
}
