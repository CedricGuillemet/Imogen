layout (std140) uniform ReactionDiffusionBlock
{
	int PassCount;
};

vec4 ReactionDiffusion()
{
	vec2 nuv = vUV;
	vec4 tex = texture(Sampler0, nuv);
	tex += texture(Sampler0, nuv + vec2(0.001, 0.));
	tex += texture(Sampler0, nuv + vec2(0., 0.001));
	tex += texture(Sampler0, nuv + vec2(-0.001, 0.));
	tex += texture(Sampler0, nuv + vec2(0., -0.001));
	tex *= 0.2;
	return tex;
}
