#define pi 3.15159
#define isqrt2 0.707106

//////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef VERTEX_SHADER

layout (std140) uniform EvaluationBlock
{
	mat4 viewRot;
	mat4 viewProjection;
	mat4 viewInverse;
	int targetIndex;
	int forcedDirty;
	int	uiPass;
	int frameNumber;
	vec4 mouse; // x,y, lbut down, rbut down
	ivec4 inputIndices[2];
	
	vec2 viewport;
} EvaluationParam;

layout(location = 0)in vec2 inUV;
layout(location = 1)in vec4 inCompute0;
layout(location = 2)in vec4 inCompute1;
layout(location = 3)in vec4 inCompute2;
layout(location = 4)in vec4 inCompute3;

out vec2 uv;
out vec4 color;
out vec3 normal;
out vec3 up;
void main() 
{
	float startWidth = 0.1;
	float endWidth = 0.0;
	vec4 i1 = mix(inCompute0, inCompute2, inUV.y);
	vec4 i2 = mix(inCompute2, inCompute3, inUV.y);
	
	vec4 bezierPos = mix(i1, i2, inUV.y);
	vec4 tgt = normalize(i2 - i1);
	
	vec3 right = normalize(cross(bezierPos.xyz - EvaluationParam.viewInverse[3].xyz, tgt.xyz));
	up = cross(right, tgt.xyz);
	
	float width = min(mix(startWidth, endWidth, inUV.y), startWidth * 0.5);
	
	vec4 pos = bezierPos + vec4(right,0.) * inUV.x * width;

	gl_Position = EvaluationParam.viewProjection * vec4(pos.xyz, 1.0); 
	
	
	normal = right;
	uv = vec2(inUV.x * 0.5 + 0.5, inUV.y);
	color = vec4(inCompute0.a, inCompute1.a, inCompute2.a, 1.);
} 
#endif

#ifdef FRAGMENT_SHADER
layout (location=0) out vec4 co;
in vec2 uv;
in vec4 color;
in vec3 normal;
in vec3 up;

void main() 
{ 
	/*float u01 = (uv.x * 2.0 - 1.0);
	vec3 n = mix(up, normal, abs(u01));
	float dnl = max(dot(normalize(n), normalize(vec3(1.))), 0.) * 0.4 + 0.6;
	co = color * dnl;
	*/
	co = color;
}
#endif