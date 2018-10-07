layout (std140) uniform SwirlCoordsBlock
{
	vec2 angles;
} SwirlCoordsParam;

vec4 Swirl()
{
    vec2 uv = vUV - vec2(0.5);
	float len = length(uv) / (SQRT2 * 0.5);
	float angle = mix(SwirlCoordsParam.angles.x, SwirlCoordsParam.angles.y, len);
	vec2 nuv = Rotate2D(uv, angle) + vec2(0.5);
	vec4 tex = texture(Sampler0, nuv);

	return tex;
}
