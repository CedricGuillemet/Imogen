$input a_texcoord0, a_color0, a_position, a_normal
$output v_texcoord0, v_color0, v_positionWorld, v_normal


#include "bgfx_shader.sh"
#include "Common.shader"

void main()
{
	if (u_mVertexSpace == 1)
    {
		gl_Position = mul(vec4(a_position.xyz, 1.0), u_modelViewProjection);
	}
	else
	{
		gl_Position = vec4(a_texcoord0.xy*2.0-1.0,0.5,1.0); 
	}
	
	v_texcoord0 = a_texcoord0;
	v_color0 = a_color0;
	v_normal = mul(vec4(a_normal, 0.0), u_model).xyz;
	v_positionWorld = mul(vec4(a_position, 1.0), u_model).xyz;
}

#if 0
#define pi 3.15159
#define isqrt2 0.707106

layout(location = 0)in vec2 inUV;
layout(location = 1)in vec4 inCompute0;
layout(location = 2)in vec4 inCompute1;
layout(location = 3)in vec4 inCompute2;
layout(location = 4)in vec4 inCompute3;
layout(location = 5)in vec4 inCompute4;
layout(location = 6)in vec4 inCompute5;
layout(location = 7)in vec4 inCompute6;
layout(location = 8)in vec4 inCompute7;
layout(location = 9)in vec4 inCompute8;
layout(location = 10)in vec4 inCompute9;
layout(location = 11)in vec4 inCompute10;
layout(location = 12)in vec4 inCompute11;
layout(location = 13)in vec4 inCompute12;
layout(location = 14)in vec4 inCompute13;
layout(location = 15)in vec4 inCompute14;

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