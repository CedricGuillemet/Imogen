layout (std140) uniform BlendBlock
{
	vec4 A;
	vec4 B;
	int op;
} BlendParam;

vec4 Blend()
{
	switch (BlendParam.op)
	{
	case 0: // add
		return texture(Sampler0, vUV) * BlendParam.A + texture(Sampler1, vUV) * BlendParam.B;
	case 1: // mul
		return texture(Sampler0, vUV) * BlendParam.A * texture(Sampler1, vUV) * BlendParam.B;
	case 2: // min
		return min(texture(Sampler0, vUV) * BlendParam.A, texture(Sampler1, vUV) * BlendParam.B);
	case 3: // max
		return max(texture(Sampler0, vUV) * BlendParam.A, texture(Sampler1, vUV) * BlendParam.B);
	}
}