layout (std140) uniform NormalMapBlock
{
	float spread;
} NormalMapParam;

vec4 NormalMap()
{
	vec3 off = vec3(-1.0,0.0,1.0) * NormalMapParam.spread;

    float s11 = texture(Sampler0, vUV).x;
    float s01 = texture(Sampler0, vUV + off.xy).x;
    float s21 = texture(Sampler0, vUV + off.zy).x;
    float s10 = texture(Sampler0, vUV + off.yx).x;
    float s12 = texture(Sampler0, vUV + off.yz).x;
    vec3 va = normalize(vec3(NormalMapParam.spread,0.0,s21-s01));
    vec3 vb = normalize(vec3(0.0,NormalMapParam.spread,s12-s10));
    vec4 bump = vec4( normalize(cross(va,vb))*0.5+0.5, s11 );
	return bump;
}
