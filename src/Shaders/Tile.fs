$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

// starting point : https://www.shadertoy.com/view/4d2Xzh
uniform vec4 translation;
uniform vec4 quincunx;
uniform vec4 noiseFactor;
uniform vec4 rotationFactor;
uniform vec4 scale;
uniform vec4 innerScale;

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
	vec2 vnoise = vec2(noise(cell) + cos(cell.y) * cellsize,
		noise(cell*cellsize) + sin(cell.x) * cellsize) * noiseFactor.x;
	float quincunxX = mod(cell.y,2.) * cellsize * quincunx.x;
    float quincunxY = mod(cell.x,2.) * cellsize * quincunx.y;
	return vnoise + vec2(quincunxX, quincunxY);
}

vec4 getCell(vec2 t, out float rad, out float idx, out vec2 uvAtCellCenter)
{
    vec2 p = floor(t);
    float nd = 1e10;
    vec2 nc;
    vec2 nq;

    {
        for(int y = -1; y < 2; y += 1)
        {
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
                    uvAtCellCenter = c / scale.x - translation.xy;
                }
            }
        }
    }
    rad = 1.0;
    idx = nq.x + nq.y;

    {
        for(int y = -1; y < 2; y += 1)
        {
            for(int x = -1; x < 2; x += 1)
            {
                if(x!=0 || y!=0)
                {

                vec2 b = vec2(float(x), float(y));
                vec2 q = b + nq;
                vec2 c = q + cellPoint(q);

                rad = min(rad, distance(nc, c) * 0.5);
                }
            }
        }
    }
    return vec4((t - nc) / rad, nc);
}

vec4 getSprite(vec2 uv, float ng)
{
	mat2 rot = mat2(vec2(cos(ng), -sin(ng)), vec2(sin(ng), cos(ng)));
	uv = mul(rot, uv);
    uv /= innerScale.x;
    uv += 0.5;
    //uv = max(min(uv, vec2(1.0)), vec2(0.0));
    return texture2D(Sampler0, uv);
}

void main()
{
	float rad, idx;
    vec2 uvAtCellCenter;
	vec2 tt = (v_texcoord0 + translation.xy) * scale.x;
    vec4 c = getCell(tt, rad, idx, uvAtCellCenter);
    vec4 multiplier = vec4(1.0, 1.0, 1.0, 1.0);
    if (u_inputIndices[0].y > -1.)
    {
        multiplier = texture2D(Sampler1, uvAtCellCenter);
    }
	vec4 spr = getSprite(c.xy, idx * rotationFactor.x) * multiplier;
	gl_FragColor = vec4(spr.xyz, 1.0);
}
