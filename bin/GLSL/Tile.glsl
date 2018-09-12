layout (std140) uniform TileBlock
{
	float scale;
	vec2 offset0;
	vec2 offset1;
	vec2 overlap;
} TileParam;

vec4 GetTile(vec2 uv)
{
    vec2 t = floor((floor(uv)+1.0)*0.5);
    uv += t *0.1;
    vec2 md = mod(uv, 2.0);
    
    if (md.x > 1.0 || md.y > 1.0)
        return vec4(0.0);
    
    return texture(Sampler0, uv);
}

vec4 Tile()
{
	vec2 nuv = vUV * TileParam.scale;

    vec4 col = GetTile(nuv + TileParam.offset0);
    col += GetTile(nuv + vec2(0.95, 0.0)+TileParam.offset0);
    col += GetTile(nuv + vec2(0.0, 0.95)+TileParam.offset1);    
    col += GetTile(nuv + vec2(0.95, 0.95)+TileParam.offset1); 
	
	return col;
}