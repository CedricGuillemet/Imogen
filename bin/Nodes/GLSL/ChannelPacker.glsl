
layout (std140) uniform ChannelPackerBlock
{
	int R;
	int G;
	int B;
	int A;
} ChannelPackerParam;

vec4 ChannelPacker()
{
    vec4 lu[4];
	lu[0]= texture(Sampler0, vUV);
	lu[1]= texture(Sampler1, vUV);
	lu[2]= texture(Sampler2, vUV);
	lu[3]= texture(Sampler3, vUV);
		
	vec4 res = vec4(lu[(ChannelPackerParam.R>>2)&3][ChannelPackerParam.R&3],
		lu[(ChannelPackerParam.G>>2)&3][ChannelPackerParam.G&3],
		lu[(ChannelPackerParam.B>>2)&3][ChannelPackerParam.B&3],
		lu[(ChannelPackerParam.A>>2)&3][ChannelPackerParam.A&3]);
		
	if (ChannelPackerParam.R>15)
		res.x = 1. - res.x;
	if (ChannelPackerParam.G>15)
		res.y = 1. - res.y;
	if (ChannelPackerParam.B>15)
		res.z = 1. - res.z;
	if (ChannelPackerParam.A>15)
		res.w = 1. - res.w;

	return res;
}
