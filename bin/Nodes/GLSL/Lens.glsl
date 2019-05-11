layout (std140) uniform LensBlock
{
	float factor;
	float vignette;
} LensParam;

vec3 Distort(vec2 uv)
{
    float distCoeff = LensParam.factor;
    uv -= 0.5;
    float r2 = dot(uv,uv);       
	float f = 1.+r2*distCoeff;
    f /= 1.+0.5*distCoeff;
    return vec3(uv*f+0.5, sqrt(r2));
}

vec4 Lens()
{
	vec3 dis = Distort(vUV.xy);
	return texture(Sampler0, dis.xy) * mix(1.0, min(1.18-dis.z, 1.0), LensParam.vignette); 
}
