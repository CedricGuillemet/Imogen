$input a_texcoord0, a_color0, a_position, a_normal
$output v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "Common.shader"

uniform vec4 u_uvTransform;

void main()
{
	if (u_target.y > 0.5)
    {
		mat4 mvp = u_worldViewProjection;
		mvp[0] *= u_uvTransform.x * 0.5;
		mvp[1] *= u_uvTransform.y * 0.5;
		gl_Position = mul(mvp, vec4(a_position.xyz, 1.0));

		vec2 clipSpaceTr = u_uvTransform.zw + u_uvTransform.xy * vec2(0.5, 0.5);
		gl_Position.xy += clipSpaceTr * gl_Position.w;
	}
	else
	{
		vec2 trPos = a_position.xy * u_uvTransform.xy + u_uvTransform.zw;
		gl_Position = vec4(trPos.xy, 0.5, 1.0); 
	}
	
	v_texcoord0 = a_texcoord0;
	v_color0 = a_color0;
	v_normal = mul(u_world, vec4(a_normal, 0.0)).xyz;
	v_positionWorld = mul(u_world, vec4(a_position, 1.0)).xyz;
}