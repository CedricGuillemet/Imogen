layout (std140) uniform BlendBlock
{
	vec4 A;
	vec4 B;
	int op;
} BlendParam;

vec4 Blend()
{
    vec4 a = texture(Sampler0, vUV) * BlendParam.A;
    vec4 b = texture(Sampler1, vUV) * BlendParam.B;
    vec4 white = vec4(1,1,1,1);
	switch (BlendParam.op)
	{
	case 0: // Add
		return a + b;
	case 1: // Multiply
		return a * b;
	case 2: // Darken
		return min(a, b);
	case 3: // Lighten
		return max(a, b);
	case 4: // Average
		return (a + b) * 0.5;
	case 5: // Screen
		return white - ((white - b) * (white - a));
	case 6: // Color Burn
		return white - (white - a) / b;
	case 7: // Color Dodge
		return a / (white - b);
	case 8: // Soft Light
		return 2.0 * a * b + a * a - 2.0 * a * a * b;
	case 9: // Subtract
		return a - b;
	case 10: // Difference
		return abs(b - a);
	case 11: // Inverse Difference
		return white - abs(white - a - b);
	case 12: // Exclusion
		return b + a - (2.0 * a * b);
	}
}