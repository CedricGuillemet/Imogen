// edited version from https://www.shadertoy.com/view/4d2Xzh
layout (std140) uniform SplatterBlock
{
	vec2 translation;
	float noiseFactor;
	float rotationFactor;
	float scale;
} SplatterParam;

float hash(float n)
{
    return fract(sin(n) * 437587.5453);
}

float noise(vec2 p)
{
    return hash(p.x + p.y * 571.0);
}

float smoothNoise2(vec2 p)
{
    vec2 p0 = floor(p + vec2(0.0, 0.0));
    vec2 p1 = floor(p + vec2(1.0, 0.0));
    vec2 p2 = floor(p + vec2(0.0, 1.0));
    vec2 p3 = floor(p + vec2(1.0, 1.0));
    vec2 pf = fract(p);
    return mix( mix(noise(p0), noise(p1), pf.x), 
              	mix(noise(p2), noise(p3), pf.x), pf.y);
}

vec2 cellPoint(vec2 cell)
{
    float cellsize = 0.5;
	return vec2(noise(cell) * SplatterParam.noiseFactor + cos(cell.y) * cellsize,
		noise(cell*cellsize) * SplatterParam.noiseFactor + sin(cell.x) * cellsize);
}

vec4 circles(vec2 t, out float rad, out float idx)
{
    vec2 p = floor(t);
    float nd = 1e10;
    vec2 nc;
    vec2 nq;

    for(int y = -1; y < 2; y += 1)
        for(int x = -1; x < 2; x += 1)
        {
            vec2 b = vec2(float(x), float(y));
            vec2 q = b + p;
            vec2 c = q + cellPoint(q);
            vec2 r = c - t;

            float d = dot(r, r);

            if(d < nd)
            {
                nd = d;
                nc = c;
                nq = q;
            }
        }

    rad = 1.0;
    idx = nq.x + nq.y;// * 119.0;

    for(int y = -1; y < 2; y += 1)
        for(int x = -1; x < 2; x += 1)
        {
            if(x==0 && y==0)
                continue;

            vec2 b = vec2(float(x), float(y));
            vec2 q = b + nq;
            vec2 c = q + cellPoint(q);

            rad = min(rad, distance(nc, c) * 0.5);
        }

    return vec4((t - nc) / rad, nc);
}

vec4 getSprite(vec2 uv, float ng)
{
	mat2 rot = mat2(vec2(cos(ng), -sin(ng)), vec2(sin(ng), cos(ng)));
	uv = rot * uv;
    uv *= 0.5;
    uv += 0.5;
    uv = max(min(uv, vec2(1.0)), vec2(0.0));
    return texture(Sampler0, uv);
}

vec4 Splatter()
{
	float rad, idx;
	vec2 tt = (vUV + SplatterParam.translation) * SplatterParam.scale;
    vec4 c = circles(tt, rad, idx);
	vec4 spr = getSprite(c.xy, idx * SplatterParam.rotationFactor);
	return vec4(spr.xyz, 1.0);
}
