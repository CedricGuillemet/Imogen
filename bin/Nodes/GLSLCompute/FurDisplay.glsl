#define pi 3.15159
#define isqrt2 0.707106
/*
layout(std140) uniform CameraUniform
{
	mat4 ViewProjection;
	mat4 ModelMatrix;
	mat4 View;
	vec4 cameraPos;
};
*/
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
	int padding;
	vec4 mouse; // x,y, lbut down, rbut down
	ivec4 inputIndices[2];
	
	vec2 viewport;
} EvaluationParam;

layout(location = 0)in vec2 inUV;
layout(location = 1)in vec4 inInstanceP0;
layout(location = 2)in vec4 inInstanceN0;
layout(location = 3)in vec4 inInstanceP1;
layout(location = 4)in vec4 inInstanceP2;

out vec2 uv;
void main() 
{
	float startWidth = 0.1;
	float endWidth = 0.0;
	vec4 i1 = mix(inInstanceP0, inInstanceP1, inUV.y);
	vec4 i2 = mix(inInstanceP1, inInstanceP2, inUV.y);
	
	vec4 bezierPos = mix(i1, i2, inUV.y);
	vec4 tgt = normalize(i2 - i1);
	
	vec3 right = normalize(cross(bezierPos.xyz - EvaluationParam.viewInverse[3].xyz, tgt.xyz));
	
	float width = mix(startWidth, endWidth, inUV.y);
	
	vec4 pos = bezierPos + vec4(right,0.) * inUV.x * width;
	gl_Position = EvaluationParam.viewProjection * vec4(pos.xyz, 1.0); 
	
	//gl_Position = vec4(inUV.xy, 0.0, 1.0);
	uv = vec2(inUV.x * 0.5 + 0.5, inUV.y);
} 
#endif

#ifdef FRAGMENT_SHADER
layout (location=0) out vec4 co;
in vec2 uv;
void main() 
{ 
	co = vec4(1.0, uv.y, 1.0, 1.0);
}
#endif