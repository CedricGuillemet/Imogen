
vec2 Rotate2D(vec2 v, float a) 
{
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

float Circle(vec2 uv, float radius, float t)
{
    float r = length(uv-vec2(0.5));
    float h = sin(acos(r/radius));
    return mix(1.0-smoothstep(radius-0.001, radius, length(uv-vec2(0.5))), h, t);
}

float Square(vec2 uv, float width)
{
	uv -= vec2(0.5);
    return 1.0-smoothstep(width-0.001, width, max(abs(uv.x), abs(uv.y)));
}

float Checker(vec2 uv)
{
	uv -= vec2(0.5);
    return mod(floor(uv.x)+floor(uv.y),2.0);
}

vec4 Transform(vec2 uv, vec2 translate, float rotate, float scale)
{
	vec2 rs = (uv+translate) * scale;   
    vec2 ro = vec2(rs.x*cos(rotate) - rs.y * sin(rotate), rs.x*sin(rotate) + rs.y * cos(rotate));
    vec2 nuv = fract(ro);
	vec4 tex = texture(Sampler0, nuv);
	return tex;
}

float Sine(vec2 uv, float freq, float angle)
{
	uv -= vec2(0.5);
	uv = vec2(cos(angle), sin(angle)) * (uv * freq * PI * 2.0);
    return cos(uv.x + uv.y) * 0.5 + 0.5;
}

vec4 SmoothStep(vec2 uv, float low, float high)
{
	vec4 tex = texture(Sampler0, uv);
	return smoothstep(low, high, tex);
}

vec4 Pixelize(vec2 uv, float scale)
{
	vec4 tex = texture(Sampler0, floor(uv*scale)/scale);
	return tex;
}

vec4 Blur(vec2 uv, float angle, float strength)
{
	vec2 dir = vec2(cos(angle), sin(angle));
	vec4 col = vec4(0.0);
	for(float i = -5.0;i<=5.0;i += 1.0)
	{
		col += texture(Sampler0, uv + dir * strength * i);
	}
	col /= 11.0;
	return col;
}

vec4 NormalMap(vec2 uv, float spread)
{
	vec3 off = vec3(-1.0,0.0,1.0) * spread;

    float s11 = texture(Sampler0, uv).x;
    float s01 = texture(Sampler0, uv + off.xy).x;
    float s21 = texture(Sampler0, uv + off.zy).x;
    float s10 = texture(Sampler0, uv + off.yx).x;
    float s12 = texture(Sampler0, uv + off.yz).x;
    vec3 va = normalize(vec3(spread,0.0,s21-s01));
    vec3 vb = normalize(vec3(0.0,spread,s12-s10));
    vec4 bump = vec4( normalize(cross(va,vb))*0.5+0.5, s11 );
	return bump;
}


vec4 MADD(vec2 uv, vec4 color0, vec4 color1)
{
    return texture(Sampler0, uv) * color0 + color1;
}

vec4 Color(vec2 uv, vec4 color)
{
	return color;
}

float Hexagon(vec2 uv)
{
	vec2 V = vec2(.866,.5);
    vec2 v = abs ( ((uv * 2.0)-1.0) * mat2( V, -V.y, V.x) );	

	return ceil( 1. - max(v.y, dot( v, V)) *1.15  );
}

vec4 Blend(vec2 uv, vec4 A, vec4 B, int op)
{
	switch (op)
	{
	case 0: // add
		return texture(Sampler0, uv) * A + texture(Sampler1, uv) * B;
	case 1: // mul
		return texture(Sampler0, uv) * A * texture(Sampler1, uv) * B;
	case 2: // min
		return min(texture(Sampler0, uv) * A, texture(Sampler1, uv) * B);
	case 3: // max
		return max(texture(Sampler0, uv) * A, texture(Sampler1, uv) * B);
	}
}

vec4 Invert(vec2 uv)
{
    return vec4(1.0) - texture(Sampler0, uv);
}

vec4 CircleSplatter(vec2 uv, vec2 distToCenter, vec2 radii, vec2 angles, float count)
{
	vec4 col = vec4(0.0);
	for (float i = 0.0 ; i < count ; i += 1.0)
	{
		float t = i/(count-0.0);
		vec2 dist = vec2(mix(distToCenter.x, distToCenter.y, t), 0.0);
		dist = Rotate2D(dist, mix(angles.x, angles.y, t));
		float radius = mix(radii.x, radii.y, t);
		col = max(col, vec4(Circle(uv-dist, radius, 0.0)));
	}
	return col;
}

float GetRamp(float v, vec2 arr[8]) 
{
    for (int i = 0;i<(arr.length()-1);i++)
    {
        if (v >= arr[i].x && v <= arr[i+1].x)
        {
            // linear
            //float t = (v-arr[i].x)/(arr[i+1].x-arr[i].x);
            // smooth
            float t = smoothstep(arr[i].x, arr[i+1].x, v);
            return mix(arr[i].y, arr[i+1].y, t);
        }
    }
    
    return 0.0;
}

vec4 Ramp(vec2 uv, vec2 ramp[8])
{
	vec4 tex = texture(Sampler0, uv);
	return tex * GetRamp(tex.x, ramp);
}

vec4 GetTile(vec2 uv)
{
    vec2 t = floor((floor(uv)+1.0)*0.5);
    uv += t *0.1;
    vec2 md = mod(uv, 2.0);
    
    if (md.x > 1.0 || md.y > 1.0)
        return vec4(0.0);
    
    return texture(Sampler0, uv);
}

vec4 Tile(vec2 uv, float scale, vec2 offset0, vec2 offset1, vec2 overlap)
{
	vec2 nuv = uv*scale;

    vec4 col = GetTile(nuv+offset0);
    col += GetTile(nuv + vec2(0.95, 0.0)+offset0);
    col += GetTile(nuv + vec2(0.0, 0.95)+offset1);    
    col += GetTile(nuv + vec2(0.95, 0.95)+offset1); 
	
	return col;
}

////////////////////////////////////////////////////////////////////////////////////////
// Normal map blending by ZigguratVertigo https://www.shadertoy.com/view/4t2SzR

#define TECHNIQUE_RNM 				 0
#define TECHNIQUE_PartialDerivatives 1
#define TECHNIQUE_Whiteout 			 2
#define TECHNIQUE_UDN				 3
#define TECHNIQUE_Unity				 4
#define TECHNIQUE_Linear		     5
#define TECHNIQUE_Overlay		     6

// RNM
vec3 NormalBlend_RNM(vec3 n1, vec3 n2)
{
    // Unpack (see article on why it's not just n*2-1)
	n1 = n1*vec3( 2,  2, 2) + vec3(-1, -1,  0);
    n2 = n2*vec3(-2, -2, 2) + vec3( 1,  1, -1);
    
    // Blend
    return n1*dot(n1, n2)/n1.z - n2;
}

// RNM - Already unpacked
vec3 NormalBlend_UnpackedRNM(vec3 n1, vec3 n2)
{
	n1 += vec3(0, 0, 1);
	n2 *= vec3(-1, -1, 1);
	
    return n1*dot(n1, n2)/n1.z - n2;
}

// Partial Derivatives
vec3 NormalBlend_PartialDerivatives(vec3 n1, vec3 n2)
{	
    // Unpack
	n1 = n1*2.0 - 1.0;
    n2 = n2*2.0 - 1.0;
    
    return normalize(vec3(n1.xy*n2.z + n2.xy*n1.z, n1.z*n2.z));
}

// Whiteout
vec3 NormalBlend_Whiteout(vec3 n1, vec3 n2)
{
    // Unpack
	n1 = n1*2.0 - 1.0;
    n2 = n2*2.0 - 1.0;
    
	return normalize(vec3(n1.xy + n2.xy, n1.z*n2.z));    
}

// UDN
vec3 NormalBlend_UDN(vec3 n1, vec3 n2)
{
    // Unpack
	n1 = n1*2.0 - 1.0;
    n2 = n2*2.0 - 1.0;    
    
	return normalize(vec3(n1.xy + n2.xy, n1.z));
}

// Unity
vec3 NormalBlend_Unity(vec3 n1, vec3 n2)
{
    // Unpack
	n1 = n1*2.0 - 1.0;
    n2 = n2*2.0 - 1.0;
    
    mat3 nBasis = mat3(vec3(n1.z, n1.y, -n1.x), // +90 degree rotation around y axis
        			   vec3(n1.x, n1.z, -n1.y), // -90 degree rotation around x axis
        			   vec3(n1.x, n1.y,  n1.z));
	
    return normalize(n2.x*nBasis[0] + n2.y*nBasis[1] + n2.z*nBasis[2]);
}

// Linear Blending
vec3 NormalBlend_Linear(vec3 n1, vec3 n2)
{
    // Unpack
	n1 = n1*2.0 - 1.0;
    n2 = n2*2.0 - 1.0;
    
	return normalize(n1 + n2);    
}

// Overlay
float overlay(float x, float y)
{
    if (x < 0.5)
        return 2.0*x*y;
    else
        return 1.0 - 2.0*(1.0 - x)*(1.0 - y);
}
vec3 NormalBlend_Overlay(vec3 n1, vec3 n2)
{
    vec3 n;
    n.x = overlay(n1.x, n2.x);
    n.y = overlay(n1.y, n2.y);
    n.z = overlay(n1.z, n2.z);

    return normalize(n*2.0 - 1.0);
}

// Combine normals
vec3 CombineNormal(vec3 n1, vec3 n2, int technique)
{
 	if (technique == TECHNIQUE_RNM)
        return NormalBlend_RNM(n1, n2);
    else if (technique == TECHNIQUE_PartialDerivatives)
        return NormalBlend_PartialDerivatives(n1, n2);
    else if (technique == TECHNIQUE_Whiteout)
        return NormalBlend_Whiteout(n1, n2);
    else if (technique == TECHNIQUE_UDN)
        return NormalBlend_UDN(n1, n2);
    else if (technique == TECHNIQUE_Unity)
        return NormalBlend_Unity(n1, n2);
    else if (technique == TECHNIQUE_Linear)
        return NormalBlend_Linear(n1, n2);
    else
        return NormalBlend_Overlay(n1, n2);
}

vec4 NormalMapBlending(vec2 uv, int technique)
{
	vec3 n1 = texture(Sampler0, uv).xyz;
	vec3 n2 = texture(Sampler1, uv).xyz;
	return vec4(CombineNormal(n1, n2, technique) * 0.5 + 0.5, 0.0);
}

// Voronoise, by iq https://www.shadertoy.com/view/Xd23Dh
vec3 hash3( vec2 p )
{
    vec3 q = vec3( dot(p,vec2(127.1,311.7)), 
				   dot(p,vec2(269.5,183.3)), 
				   dot(p,vec2(419.2,371.9)) );
	return fract(sin(q)*43758.5453);
}

float iqnoise(vec2 uv, float size, float u, float v )
{
    vec2 p = floor(uv*size);
    vec2 f = fract(uv*size);
		
	float k = 1.0+63.0*pow(1.0-v,4.0);
	
	float va = 0.0;
	float wt = 0.0;
    for( int j=-2; j<=2; j++ )
    for( int i=-2; i<=2; i++ )
    {
        vec2 g = vec2( float(i),float(j) );
		vec3 o = hash3( p + g )*vec3(u,u,1.0);
		vec2 r = g - f + o.xy;
		float d = dot(r,r);
		float ww = pow( 1.0-smoothstep(0.0,1.414,sqrt(d)), k );
		va += o.z*ww;
		wt += ww;
    }
	
    return va/wt;
}