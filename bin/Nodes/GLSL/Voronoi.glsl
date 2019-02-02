layout (std140) uniform VoronoiBlock
{
	int pointCount;
	float seed;
	float distanceBlend;
	float squareWidth;
} VoronoiParam;

float rand(float n){return fract(sin(n) * 43758.5453123);}

float sdAxisAlignedRect(vec2 uv, vec2 tl, vec2 br)
{
    vec2 d = max(tl-uv, uv-br);
    return length(max(vec2(0.0), d)) + min(0.0, max(d.x, d.y));
}

float sdAxisAlignedRectManhattan(vec2 uv, vec2 tl, vec2 br)
{
    vec2 d = max(tl-uv, uv-br);
    vec2 dif = vec2(max(vec2(0.0), d)) + min(0.0, max(d.x, d.y));
    return max(dif.x, dif.y);
}

float Voronoi()
{
    float col = 1.f;
    float minDist = 10.;
    for(int i = 0;i<VoronoiParam.pointCount;i++)
    {
        float f = float(i) * VoronoiParam.seed;
        vec4 r = vec4(rand(f), rand(f*1.2721), rand(f*7.8273) * VoronoiParam.squareWidth, rand(f*7.8273) * 0.9 + 0.1);        
        
        float d = mix(sdAxisAlignedRect(vUV, r.xy-r.zz, r.xy+r.zz),
        	sdAxisAlignedRectManhattan(vUV, r.xy-r.zz, r.xy+r.zz), VoronoiParam.distanceBlend);
        
        if (d < minDist)
        {
        	minDist = d;
            col = r.w;
        }
    }        

    return col;
}