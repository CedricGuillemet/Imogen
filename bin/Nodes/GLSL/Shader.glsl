#define PI 3.14159265359
#define SQRT2 1.414213562373095

#define TwoPI (PI*2)


layout (std140) uniform EvaluationBlock
{
	mat4 viewRot;
	mat4 viewProjection;
	mat4 viewInverse;
	vec4 viewport;
	
	int targetIndex;
	int forcedDirty;
	int	uiPass;
	int passNumber;
	
	vec4 mouse; // x,y, lbut down, rbut down
	ivec4 inputIndices[2];
	
	int frame;
	int localFrame;
	int mVertexSpace;
	int dummy;
} EvaluationParam;

#ifdef VERTEX_SHADER

layout(location = 0)in vec2 inUV;
layout(location = 1)in vec4 inColor;
layout(location = 2)in vec3 inPosition;
layout(location = 3)in vec3 inNormal;

out vec2 vUV;
out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec4 vColor;
void main()
{
	if (EvaluationParam.mVertexSpace == 1)
    {
		gl_Position = EvaluationParam.viewProjection * vec4(inPosition.xyz, 1.0);
	}
	else
	{
		gl_Position = vec4(inUV.xy*2.0-1.0,0.5,1.0); 
	}
	
	vUV = inUV;
	vColor = inColor;
	vWorldNormal = inNormal;
	vWorldPosition = inPosition;
}

#endif


#ifdef FRAGMENT_SHADER

struct Camera
{
	vec4 pos;
	vec4 dir;
	vec4 up;
	vec4 lens;
};

layout(location=0) out vec4 outPixDiffuse;
in vec2 vUV;
in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec4 vColor;

uniform sampler2D Sampler0;
uniform sampler2D Sampler1;
uniform sampler2D Sampler2;
uniform sampler2D Sampler3;
uniform sampler2D Sampler4;
uniform sampler2D Sampler5;
uniform sampler2D Sampler6;
uniform sampler2D Sampler7;
uniform samplerCube CubeSampler0;
uniform samplerCube CubeSampler1;
uniform samplerCube CubeSampler2;
uniform samplerCube CubeSampler3;
uniform samplerCube CubeSampler4;
uniform samplerCube CubeSampler5;
uniform samplerCube CubeSampler6;
uniform samplerCube CubeSampler7;

vec2 Rotate2D(vec2 v, float a) 
{
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}
vec3 InvertCubeY(vec3 dir)
{
	return vec3(dir.x, -dir.y, dir.z);
}
float Circle(vec2 uv, float radius, float t)
{
    float r = length(uv-vec2(0.5));
    float h = sin(acos(r/radius));
    return mix(1.0-smoothstep(radius-0.001, radius, length(uv-vec2(0.5))), h, t);
}

vec4 boxmap( sampler2D sam, in vec3 p, in vec3 n, in float k )
{
    vec3 m = pow( abs(n), vec3(k) );
	vec4 x = texture( sam, p.yz );
	vec4 y = texture( sam, p.zx );
	vec4 z = texture( sam, p.xy );
	return (x*m.x + y*m.y + z*m.z)/(m.x+m.y+m.z);
}

vec2 envMapEquirect(vec3 wcNormal, float flipEnvMap) {
  //I assume envMap texture has been flipped the WebGL way (pixel 0,0 is a the bottom)
  //therefore we flip wcNorma.y as acos(1) = 0
  float phi = acos(-wcNormal.y);
  float theta = atan(flipEnvMap * wcNormal.x, wcNormal.z) + PI;
  return vec2(theta / TwoPI, 1.0 - phi / PI);
}

vec2 envMapEquirect(vec3 wcNormal) {
    //-1.0 for left handed coordinate system oriented texture (usual case)
    return envMapEquirect(wcNormal, -1.0);
}

float saturate( float x )
{
    return clamp( x, 0., 1. );
}

vec3 saturate( vec3 x )
{
    return clamp( x, vec3( 0. ), vec3( 1. ) );
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


__NODE__



void main() 
{ 
	outPixDiffuse = vec4(__FUNCTION__);
}

#endif
