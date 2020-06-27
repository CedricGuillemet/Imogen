$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

void main()
{
	vec3 lightdir = normalize(vec3(1.0, 1.0, 1.0));
	float dt = max(dot(lightdir, normalize(v_normal)), 0.5);
	dt -= max(dot(-lightdir, normalize(v_normal)), 0.0) * 0.3;
    gl_FragColor = vec4(dt, dt, dt, 1.0);
}
