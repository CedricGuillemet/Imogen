layout (std140) uniform KaleidoscopeBlock
{
	vec2 center;
	float startAngle;
	int splits;
	int sym;
} KaleidoscopeParam;

vec4 Kaleidoscope()
{
	vec2 center = vec2(0.5);
	
	vec2 uv = vUV - KaleidoscopeParam.center;
	float l = length(uv);
	
	float ng = atan(uv.y, uv.x) + PI - KaleidoscopeParam.startAngle;
	float modulo = (2.* PI) / float(KaleidoscopeParam.splits + 1);
	float count = mod((ng / modulo) *2.0, 2.0);
	ng = mod(ng, modulo);
	if ((KaleidoscopeParam.sym != 0) && (count>1.))
	{
		ng = modulo - mod(ng, modulo);
	}
	else
	{
		ng = mod(ng, modulo);
	}
	ng += KaleidoscopeParam.startAngle;
	vec2 uv2 = vec2(-cos(-ng), sin(-ng)) * l + KaleidoscopeParam.center;
	vec4 tex = texture(Sampler0, uv2);
	return tex;
}
