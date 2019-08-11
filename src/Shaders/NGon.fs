$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

uniform vec4 u_sides;
uniform vec4 u_radius;
uniform vec4 u_t;
uniform vec4 u_angle;

void main()
{
    vec2 p = v_texcoord0 - vec2(0.5, 0.5);
    
    vec2 d = vec2(cos(u_angle.x), sin(u_angle.x));
    float ng = 0.0;
    float col = 0.0;
    
    int sideCount = int(u_sides.x);
    for (int i=0;i<64;i++)
    {
        vec2 rd = Rotate2D(d, ng);
     	ng += (PI * 2.0) / u_sides.x;
        float d = smoothstep(mix(-u_radius.x, 0.0, u_t.x), 0.001, dot(p,rd)-u_radius.x) * ((i>= sideCount)?0.:1.);
        col = max(col, d);
    }
    col = 1.0 - col;
    gl_FragColor = vec4(col, col, col, col);
}
