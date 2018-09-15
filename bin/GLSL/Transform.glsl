layout (std140) uniform TransformBlock
{
	vec2 translate;
	float rotate;
	float scale;
} TransformParam;

vec4 Transform()
{
	vec2 rs = (vUV+TransformParam.translate) * TransformParam.scale;   
    vec2 ro = vec2(rs.x*cos(TransformParam.rotate) - rs.y * sin(TransformParam.rotate), rs.x*sin(TransformParam.rotate) + rs.y * cos(TransformParam.rotate));
    vec2 nuv = ro;//fract(ro);
	vec4 tex = texture(Sampler0, nuv);
	return tex;
}
