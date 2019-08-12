#define PI 3.14159265359
#define SQRT2 1.414213562373095

#define TwoPI (PI*2.0)


uniform mat4 u_viewRot;
uniform mat4 u_viewProjection;
uniform mat4 u_viewInverse;
uniform mat4 u_world;
uniform mat4 u_worldViewProjection;
uniform vec4 u_viewport;


uniform vec4 u_mouse; // x,y, lbut down, rbut down
uniform vec4 u_keyModifier; // ctrl, alt, shift
uniform vec4 u_inputIndices[2];

uniform vec4 u_pass; // uiPass, passNumber, frame, localFrame
uniform vec4 u_target; // targetIndex, vertexSpace, mipmapNumber, mipmapCount


vec2 Rotate2D(vec2 v, float a) 
{
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return mul(v, m);
}
vec3 InvertCubeY(vec3 dir)
{
	return vec3(dir.x, -dir.y, dir.z);
}
float Circle(vec2 uv, float radius, float t)
{
    float r = length(uv-vec2(0.5, 0.5));
    float h = sin(acos(r/radius));
    return mix(1.0-smoothstep(radius-0.001, radius, length(uv-vec2(0.5, 0.5))), h, t);
}

vec4 boxmap( sampler2D sam, in vec3 p, in vec3 n, in float k )
{
    vec3 m = pow( abs(n), vec3(k, k, k) );
	vec4 x = texture2D( sam, p.yz );
	vec4 y = texture2D( sam, p.zx );
	vec4 z = texture2D( sam, p.xy );
	return (x*m.x + y*m.y + z*m.z)/(m.x+m.y+m.z);
}

vec2 boxUV(vec3 p, vec3 n)
{
	vec2 uv = p.xy;
	uv = mix(uv, p.zy*sign(n.x), (abs(n.x)>0.5)?1.0:0.0 );
	uv = mix(uv, p.zx*sign(n.y), (abs(n.y)>0.5)?1.0:0.0 );
	return uv;
}

vec2 envMapEquirect(vec3 wcNormal, float flipEnvMap) {
  //I assume envMap texture2D has been flipped the WebGL way (pixel 0,0 is a the bottom)
  //therefore we flip wcNorma.y as acos(1) = 0
  float phi = acos(-wcNormal.y);
  float theta = atan2(flipEnvMap * wcNormal.x, wcNormal.z) + PI;
  return vec2(theta / TwoPI, 1.0 - phi / PI);
}

vec2 envMapEquirect(vec3 wcNormal) {
    //-1.0 for left handed coordinate system oriented texture (usual case)
    return envMapEquirect(wcNormal, -1.0);
}

float Smooth( float x )
{
	return smoothstep( 0., 1., saturate( x ) );   
}

// distance functions

float Cylinder( vec3 p, float r, float height ) 
{
	float d = length( p.xz ) - r;
	d = max( d, abs( p.y ) - height );
	return d;
}

float Substract( float a, float b )
{
    return max( a, -b );
}

float SubstractRound( float a, float b, float r ) 
{
	vec2 u = max( vec2( r + a, r - b ), vec2( 0.0, 0.0 ) );
	return min( -r, max( a, -b ) ) + length( u );
}

float Union( float a, float b )
{
    return min( a, b );
}

float Box( vec3 p, vec3 b )
{
	vec3 d = abs( p ) - b;
	return min( max( d.x, max( d.y, d.z ) ), 0.0 ) + length( max( d, 0.0 ) );
}

float Sphere( vec3 p, float s )
{
	return length( p ) - s;
}

float Torus( vec3 p, float sr, float lr )
{
	return length( vec2( length( p.xz ) - lr, p.y ) ) - sr;
}

float Disc( vec3 p, float r, float t ) 
{
	float l = length( p.xz ) - r;
	return l < 0. ? abs( p.y ) - t : length( vec2( p.y, l ) ) - t;
}

float UnionRound( float a, float b, float k )
{
    float h = clamp( 0.5 + 0.5 * ( b - a ) / k, 0.0, 1.0 );
    return mix( b, a, h ) - k * h * ( 1.0 - h );
}

float sdRoundBox( vec3 p, vec3 b, float r )
{
  vec3 d = abs(p) - b;
  return length(max(d,0.0)) - r
         + min(max(d.x,max(d.y,d.z)),0.0); // remove this line for an only partially signed sdf 
}
