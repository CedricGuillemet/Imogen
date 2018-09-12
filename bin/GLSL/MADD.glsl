layout (std140) uniform MADDBlock
{
	vec4 color0;
	vec4 color1;
} MADDParam;

vec4 MADD()
{
    return texture(Sampler0, vUV) * MADDParam.color0 + MADDParam.color1;
}
