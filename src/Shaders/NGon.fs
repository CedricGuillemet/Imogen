$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

int u_sides;
float u_radius;
float u_t;
float u_angle;

void main()
{
    vec2 p = v_texcoord0 - vec2(0.5, 0.5);
    
    vec2 d = vec2(cos(u_angle), sin(u_angle));
    float ng = 0.0;
    float col = 0.0;
    
    for(int i = 0;i<u_sides;i++)
    {
        vec2 rd = Rotate2D(d, ng);
     	ng += 3.14159*2.0 / float(u_sides);
        float d = smoothstep(mix(-u_radius, 0.0, u_t), 0.001, dot(p,rd)-u_radius);
        col = max(col, d);
    }
	col = 1.0 - col;
	gl_FragColor = vec4(col, col, col, col);
}
