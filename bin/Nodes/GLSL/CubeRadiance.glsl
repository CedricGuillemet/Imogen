
layout (std140) uniform CubeRadianceBlock
{
	int size;
};

vec4 CubeRadiance()
{
    return vec4(1.0,0.4,0.1,1.0);
}
