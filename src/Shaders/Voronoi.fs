$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

uniform vec4 pointCount;
uniform vec4 seed;
uniform vec4 distanceBlend;
uniform vec4 squareWidth;

float rand(float n){return fract(sin(n) * 43758.5453123);}

float sdAxisAlignedRect(vec2 uv, vec2 tl, vec2 br)
{
    vec2 d = max(tl-uv, uv-br);
    return length(max(vec2(0.0, 0.0), d)) + min(0.0, max(d.x, d.y));
}

float sdAxisAlignedRectManhattan(vec2 uv, vec2 tl, vec2 br)
{
    vec2 d = max(tl-uv, uv-br);
    vec2 dif = vec2(max(vec2(0.0, 0.0), d)) + min(0.0, max(d.x, d.y));
    return max(dif.x, dif.y);
}

void main()
{
    float col = 1.f;
    float minDist = 10.;
    int ipointCount = int(pointCount.x);
    for(int i = 0;i<64;i++)
    {
        if (i<ipointCount)
        {
            float f = float(i) * seed.x;
            vec4 r = vec4(rand(f), rand(f*1.2721), rand(f*7.8273) * squareWidth.x, rand(f*7.8273) * 0.9 + 0.1);        
            
            float d = mix(sdAxisAlignedRect(v_texcoord0, r.xy-r.zz, r.xy+r.zz),
                sdAxisAlignedRectManhattan(v_texcoord0, r.xy-r.zz, r.xy+r.zz), distanceBlend.x);
            
            if (d < minDist)
            {
                minDist = d;
                col = r.w;
            }
        }
    }        

    gl_FragColor = vec4(col, col, col, col);
}