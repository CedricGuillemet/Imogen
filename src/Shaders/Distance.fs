$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

SAMPLER2D(Sampler0, 0);

uniform vec4 passCount;

// source
// https://www.shadertoy.com/view/ldVGWc

vec4 Pack(vec4 coord)
{
    float s = float(256.0/*textureSize(Sampler0, 0).x*/);
    return coord / s;
}

vec4 Unpack(vec4 v)
{
    float s = float(256.0/*textureSize(Sampler0, 0).x*/);
    return v * s;
}

vec4 StepJFA (vec2 fragCoord, float level, float c_maxSteps)
{
    level = clamp(level, 0.0, c_maxSteps);
    float stepwidth = floor(exp2(c_maxSteps - level)+0.5);
    
    float bestDistanceIn = 9999.0;
    float bestDistanceOut = 9999.0;
    vec4 bestCoord = vec4(0.0, 0.0, 0.0, 0.0);
    
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 sampleCoord = fragCoord + vec2(x,y) * stepwidth;
            
            vec4 data = texture2D(Sampler0, sampleCoord / vec2(256.0, 256.0/*textureSize(Sampler0, 0).xy*/));
            vec4 seedCoord = Unpack(data);
            
            float dist = length(seedCoord.xy - fragCoord);
            if (dist < bestDistanceIn)
            {
                bestDistanceIn = dist;
                bestCoord.xy = seedCoord.xy;
            }
            dist = length(seedCoord.zw - fragCoord);
            if (dist < bestDistanceOut)
            {
                bestDistanceOut = dist;
                bestCoord.zw = seedCoord.zw;
            }
        }
    }
    
    return Pack(bestCoord);
}

void main()
{
    vec4 res;
    if (u_pass.x == 0.)
    {
        float v = texture2D(Sampler0, v_texcoord0).x;
        if (v > 0.5)
        {
            res = Pack(vec4(gl_FragCoord.xy, vec2(10000.0, 10000.0)));
        }
        else
        {
            res = Pack(vec4(vec2(10000.0, 10000.0), gl_FragCoord.xy));
        }
    }
    else if (u_pass.x != (passCount.x - 1.))
    {
        res = StepJFA(gl_FragCoord.xy, u_pass.x-1., (passCount.x-2.));
    }
    else
    {
        // display result
        vec4 data = texture2D(Sampler0, v_texcoord0);
        vec4 seedCoord = Unpack(data);
        
        float distIn = length(seedCoord.xy - gl_FragCoord.xy) / 128.0;
        float distOut = length(seedCoord.zw - gl_FragCoord.xy) / 128.0;
        
        float d = 0.5 + distOut * 0.5 - distIn * 0.5;
        res = vec4(d, d, d, 1.0); 
    }
    gl_FragColor = res;
}
