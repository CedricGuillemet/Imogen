layout (std140) uniform TransformBlock
{
	vec2 translate;
	vec2 scale;
	float rotate;
};

vec4 Transform()
{
	vec2 rs = (vUV+translate) * scale;   
	rs -= 0.5;
    vec2 ro = vec2(rs.x*cos(rotate) - rs.y * sin(rotate), rs.x*sin(rotate) + rs.y * cos(rotate));
	ro += 0.5;
    vec2 nuv = ro;//fract(ro);
	vec4 tex = texture(Sampler0, nuv);
	return tex;
}
