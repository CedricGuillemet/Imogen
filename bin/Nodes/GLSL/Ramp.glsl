float GetRamp(float v, vec2 arr[8]) 
{
    for (int i = 0;i<(arr.length()-1);i++)
    {
        if (v >= arr[i].x && v <= arr[i+1].x)
        {
            // linear
            //float t = (v-arr[i].x)/(arr[i+1].x-arr[i].x);
            // smooth
            float t = smoothstep(arr[i].x, arr[i+1].x, v);
            return mix(arr[i].y, arr[i+1].y, t);
        }
    }
    
    return 0.0;
}

layout (std140) uniform RampBlock
{
	vec2 ramp[8];
} RampParam;

vec4 Ramp()
{
	vec4 tex = texture(Sampler0, vUV);
	if (EvaluationParam.inputIndices[0].y > -1.)
		return texture(Sampler1, vec2(tex.x, 0.5));
	else
		return tex * GetRamp(tex.x, RampParam.ramp);
}