layout (std140) uniform CubemapFilterBlock
{
	int lightingModel;
	int excludeBase;
	int glossScale;
	int glossBias;
	int faceSize;
};

vec4 CubemapFilter()
{
	return vec4(1.0,1.0,0.0,1.0);
}
