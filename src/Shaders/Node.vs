$input a_texcoord0, a_color0, a_position, a_normal
$output v_texcoord0, v_color0, v_positionWorld, v_normal


#include "bgfx_shader.sh"
#include "Common.shader"

vec4 u_uvTransform;

void main()
{
	if (u_target.y == 1.)
    {
		gl_Position = mul(vec4(a_position.xyz, 1.0), u_worldViewProjection);
	}
	else
	{
		vec2 trUV = a_texcoord0.xy * u_uvTransform.xy + u_uvTransform.zw;
		gl_Position = vec4(trUV.xy, 0.5, 1.0); 
	}
	
	v_texcoord0 = a_texcoord0;
	v_color0 = a_color0;
	v_normal = mul(vec4(a_normal, 0.0), u_world).xyz;
	v_positionWorld = mul(vec4(a_position, 1.0), u_world).xyz;
}