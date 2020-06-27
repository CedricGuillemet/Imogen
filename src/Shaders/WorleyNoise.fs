$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

// https://github.com/Erkaman/glsl-worley
uniform vec4 translation;
uniform vec4 scale;
uniform vec4 jitter;
uniform vec4 manhattan;

// Permutation polynomial: (34x^2 + x) mod 289
vec3 permute(vec3 x) 
{
	return mod((34.0 * x + 1.0) * x, 289.0);
}

vec3 dist(vec3 x, vec3 y,  bool manhattanDistance) 
{
  return manhattanDistance ?  abs(x) + abs(y) :  (x * x + y * y);
}

float worley(vec2 P, float jitter, bool manhattanDistance) 
{
	float K= 0.142857142857; // 1/7
	float Ko= 0.428571428571 ;// 3/7
  	vec2 Pi = mod(floor(P), 289.0);
   	vec2 Pf = fract(P);
  	vec3 oi = vec3(-1.0, 0.0, 1.0);
  	vec3 of = vec3(-0.5, 0.5, 1.5);
  	vec3 px = permute(Pi.x + oi);
  	vec3 p = permute(px.x + Pi.y + oi); // p11, p12, p13
  	vec3 ox = fract(p*K) - Ko;
  	vec3 oy = mod(floor(p*K),7.0)*K - Ko;
  	vec3 dx = Pf.x + 0.5 + jitter*ox;
  	vec3 dy = Pf.y - of + jitter*oy;
  	vec3 d1 = dist(dx,dy, manhattanDistance); // d11, d12 and d13, squared
  	p = permute(px.y + Pi.y + oi); // p21, p22, p23
  	ox = fract(p*K) - Ko;
  	oy = mod(floor(p*K),7.0)*K - Ko;
  	dx = Pf.x - 0.5 + jitter*ox;
  	dy = Pf.y - of + jitter*oy;
  	vec3 d2 = dist(dx,dy, manhattanDistance); // d21, d22 and d23, squared
  	p = permute(px.z + Pi.y + oi); // p31, p32, p33
  	ox = fract(p*K) - Ko;
  	oy = mod(floor(p*K),7.0)*K - Ko;
  	dx = Pf.x - 1.5 + jitter*ox;
  	dy = Pf.y - of + jitter*oy;
  	vec3 d3 = dist(dx,dy, manhattanDistance); // d31, d32 and d33, squared
  	// Sort out the two smallest distances (F1, F2)
  	vec3 d1a = min(d1, d2);
  	d2 = max(d1, d2); // Swap to keep candidates for F2
  	d2 = min(d2, d3); // neither F1 nor F2 are now in d3
  	d1 = min(d1a, d2); // F1 is now in d1
  	d2 = max(d1a, d2); // Swap to keep candidates for F2
  	d1.xy = (d1.x < d1.y) ? d1.xy : d1.yx; // Swap if smaller
  	d1.xz = (d1.x < d1.z) ? d1.xz : d1.zx; // F1 is in d1.x
  	d1.yz = min(d1.yz, d2.yz); // F2 is now not in d2.yz
  	d1.y = min(d1.y, d1.z); // nor in  d1.z
  	d1.y = min(d1.y, d2.x); // F2 is in d1.y, we're done.
  	return sqrt(d1.x);
}

void main()
{
	float w = worley((v_texcoord0 + translation.xy) * scale.x, jitter.x, manhattan.x > 0.001);
	gl_FragColor = vec4(w, w, w, w);
}

