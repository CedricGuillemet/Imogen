$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

uniform vec4 sides;
uniform vec4 radius;
uniform vec4 T;
uniform vec4 startAngle;

void main()
{
    vec2 p = v_texcoord0 - vec2(0.5, 0.5);
    
    vec2 d = vec2(cos(startAngle.x), sin(startAngle.x));
    float ng = 0.0;
    float col = 0.0;
    
    int sideCount = int(sides.x);
    for (int i=0;i<64;i++)
    {
        vec2 rd = Rotate2D(d, ng);
     	ng += (PI * 2.0) / sides.x;
        float d = smoothstep(mix(-radius.x, 0.0, T.x), 0.001, dot(p,rd)-radius.x) * ((i>= sideCount)?0.:1.);
        col = max(col, d);
    }
    col = 1.0 - col;
    gl_FragColor = vec4(col, col, col, col);
}
