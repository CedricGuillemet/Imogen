layout (std140) uniform DistanceBlock
{
	int passCount;
};

// source
// https://www.shadertoy.com/view/ldVGWc

vec4 Pack(vec4 coord)
{
    float s = textureSize(Sampler0, 0).x;
    return coord / s;
}

vec4 Unpack(vec4 v)
{
    float s = textureSize(Sampler0, 0).x;
    return v * s;
}

vec4 StepJFA (in vec2 fragCoord, in float level, float c_maxSteps)
{
    level = clamp(level, 0.0, c_maxSteps);
    float stepwidth = floor(exp2(c_maxSteps - level)+0.5);
    
    float bestDistanceIn = 9999.0;
    float bestDistanceOut = 9999.0;
    vec4 bestCoord = vec4(0.0);
    
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 sampleCoord = fragCoord + vec2(x,y) * stepwidth;
            
            vec4 data = texture(Sampler0, sampleCoord / textureSize(Sampler0, 0).xy);
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

vec4 Distance()
{
    if (EvaluationParam.passNumber == 0)
    {
        float v = texture(Sampler0, vUV).x;
        if (v > 0.5)
        {
            return Pack(vec4(gl_FragCoord.xy, vec2(10000.0)));
        }
        else
        {
            return Pack(vec4(vec2(10000.0), gl_FragCoord.xy));
        }
    }
    else if (EvaluationParam.passNumber != (passCount - 1))
    {
        return StepJFA(gl_FragCoord.xy, float(EvaluationParam.passNumber-1), float(passCount-2));
    }
    else
    {
        // display result
        vec4 data = texture(Sampler0, vUV);
        vec4 seedCoord = Unpack(data);
        
        float distIn = length(seedCoord.xy - gl_FragCoord.xy) / 128.0;
        float distOut = length(seedCoord.zw - gl_FragCoord.xy) / 128.0;
        
        float d = 0.5 + distOut * 0.5 - distIn * 0.5;
        return vec4(d, d, d, 1.0); 
    }
}
