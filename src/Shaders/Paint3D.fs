$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

int u_size; // 1<<(size+8)

vec4 brushSample(vec2 uv, float radius)
{
	vec2 nuv = (uv) / radius + vec2(0.5, 0.5);
	float alphaMul = min(min(step(0.0, nuv.x), step(nuv.x, 1.0)), min(step(0.0, nuv.y), step(nuv.y, 1.0)));
	return texture2D(Sampler1, nuv) * alphaMul;
}

vec4 brushSampleMouse(vec3 positionWorld)
{
	vec4 wPos = mul(u_viewProjection, vec4(positionWorld, 1.0));
	mat4 projMat = mat4(
		vec4(1.0, 0.0, 0.0, 0.0),
		vec4(0.0, 1.0, 0.0, 0.0),
		vec4(0.0, 0.0, 1.0, 0.0),
		vec4(-(u_mouse.xy * 2.0 - 1.0), 0.0, 1.0));
	
	vec4 projUV = mul(projMat, wPos);
	return brushSample(projUV.xy/projUV.w, 0.3);
}

void main()
{
	if (u_pass.x == 1)
	{
		vec3 lightdir = normalize(vec3(1.0, 1.0, 1.0));
		float dt = max(dot(lightdir, normalize(v_normal)), 0.5);
		vec4 tex = texture2D(Sampler0, v_texcoord0) * dt;
		
		
		gl_FragColor = vec4(tex.xyz, 1.0) + brushSampleMouse(v_positionWorld) * vec4(1.0, 0.0, 0.0, 1.0);
	}	
	else
	{
		vec4 res = vec4(0.0, 0.0, 0.0, 0.0);
		if (u_keyModifier.x != 0. || u_keyModifier.y != 0. || u_keyModifier.z != 0.)
		{
			gl_FragColor =  res;
			return;
		}
		// paint pass
		if (u_mouse.z > 0.0)
		{
			res = brushSampleMouse(v_positionWorld);
		}
		if (u_mouse.w > 0.0)
		{
			res = brushSampleMouse(v_positionWorld);
			res.xyz = vec3(0.0, 0.0, 0.0);
		}
		gl_FragColor = vec4(res.xyz*res.w, res.w);	
	}
}