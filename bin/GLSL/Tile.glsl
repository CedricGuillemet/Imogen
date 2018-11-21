layout (std140) uniform TileBlock
{
	vec2 offset0;
	vec2 offset1;
	vec2 overlap;
	float scale;
} TileParam;

vec4 GetTile0(vec2 uv)
{
    if (uv.x > 1.0 || uv.y > 1.0 || uv.x <0.0 || uv.y<0.0)
        return vec4(0.0);
    
    return texture(Sampler0, uv);
}

vec2 GetOffset(vec2 uv)
{
	float o = float(int(floor(uv.y))&1) * TileParam.offset0.x;
	//float o = floor(fract(uv.y/2.0)*4.0) * TileParam.offset0.x * 2.0;
	return vec2(o,0.);
}

vec4 GetTile(vec2 uv)
{
	vec2 cellSize = vec2(1.0) - TileParam.overlap;

	vec4 c = vec4(0.0);

	for (int y = -1;y<2;y++)
	{
		for (int x = -1;x<2;x++)
		{
			vec2 cell0 = uv - (fract(uv/cellSize) + vec2(float(x), float(y))) * cellSize;
			vec2 cell1 = (floor(uv/cellSize) + vec2(float(x), float(y)));// * cellSize;
			vec4 multiplier = vec4(1.0);
			//if (EvaluationParam.inputIndices[1] > -1.)
				multiplier = texture(Sampler1, cell0/TileParam.scale);
			c += GetTile0(uv - cell0 + GetOffset(cell1)) * multiplier;
		}
	}

	return c;
}

vec4 Tile()
{
	vec2 nuv = vUV * TileParam.scale;

    vec4 col = GetTile(nuv);
	/*
		return vec4(0.0,1.0,0.0,1.0);
	else
		return vec4(1.0,0.0,0.0,1.0);
	*/
	return col;
}