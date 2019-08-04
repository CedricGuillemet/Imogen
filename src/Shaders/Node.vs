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