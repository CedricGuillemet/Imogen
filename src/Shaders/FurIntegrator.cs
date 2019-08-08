#include "bgfx_compute.sh"
/*
layout(location = 0)in vec4 inCompute0;
layout(location = 1)in vec4 inCompute1;
layout(location = 2)in vec4 inCompute2;
layout(location = 3)in vec4 inCompute3;
layout(location = 4)in vec4 inCompute4;
layout(location = 5)in vec4 inCompute5;
layout(location = 6)in vec4 inCompute6;
layout(location = 7)in vec4 inCompute7;
layout(location = 8)in vec4 inCompute8;
layout(location = 9)in vec4 inCompute9;
layout(location = 10)in vec4 inCompute10;
layout(location = 11)in vec4 inCompute11;
layout(location = 12)in vec4 inCompute12;
layout(location = 13)in vec4 inCompute13;
layout(location = 14)in vec4 inCompute14;

out vec4 outCompute0;
out vec4 outCompute1;
out vec4 outCompute2;
out vec4 outCompute3;
out vec4 outCompute4;
out vec4 outCompute5;
out vec4 outCompute6;
out vec4 outCompute7;
out vec4 outCompute8;
out vec4 outCompute9;
out vec4 outCompute10;
out vec4 outCompute11;
out vec4 outCompute12;
out vec4 outCompute13;
out vec4 outCompute14;

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
	int frame;
	int localFrame;
} EvaluationParam;

*/
NUM_THREADS(16, 16, 1)
void main()
{
	/*
	vec3 norm0 = inCompute1.xyz;
	float stifness = 1.;
	
	float frm = float(EvaluationParam.frame);
	float len = inCompute3.w;
	outCompute0 = inCompute0;// + vec4(0., 0.01, 0., 0.);
	outCompute1 = inCompute1;// + vec4(0.01, 0., 0., 0.);
	
	vec3 idealPos1 = inCompute0.xyz + norm0 * len + vec3(0.,-1.,0.) * len * 0.4;
	vec3 curSeg = inCompute2.xyz - inCompute0.xyz;
	vec3 idealPos2 = inCompute2.xyz + normalize(curSeg) * len  + vec3(0.,-1.,0.) * len * 0.8;
	
	outCompute2 = vec4(mix(inCompute2.xyz, idealPos1, stifness), inCompute2.w);
	outCompute3 = vec4(mix(inCompute3.xyz, idealPos2, stifness), inCompute3.w);
	
	outCompute4 = vec4(0.);
	outCompute5 = vec4(0.);
	outCompute6 = vec4(0.);
	outCompute7 = vec4(0.);
	outCompute8 = vec4(0.);
	outCompute9 = vec4(0.);
	outCompute10 = vec4(0.);
	outCompute11 = vec4(0.);
	outCompute12 = vec4(0.);
	outCompute13 = vec4(0.);
	outCompute14 = vec4(0.);
	*/
}