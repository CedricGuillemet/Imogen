$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

void main() 
{ 
	/*float u01 = (uv.x * 2.0 - 1.0);
	vec3 n = mix(up, normal, abs(u01));
	float dnl = max(dot(normalize(n), normalize(vec3(1.))), 0.) * 0.4 + 0.6;
	co = v_color0 * dnl;
	*/
	gl_FragColor = v_color0;
}
