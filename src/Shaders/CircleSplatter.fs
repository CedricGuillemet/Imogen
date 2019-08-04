$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

vec2 u_distToCenter;
vec2 u_radii;
vec2 u_angles;
float u_count;

void main()
{
	vec4 col = vec4(0.0, 0.0, 0.0, 0.0);
	for (float i = 0.0 ; i < u_count ; i += 1.0)
	{
		float t = i/(u_count-0.0);
		vec2 dist = vec2(mix(u_distToCenter.x, u_distToCenter.y, t), 0.0);
		dist = Rotate2D(dist, mix(u_angles.x, u_angles.y, t));
		float radius = mix(u_radii.x, u_radii.y, t);
		float c = Circle(v_texcoord0-dist, radius, 0.0);
		col = max(col, vec4(c, c, c, c));
	}
	gl_FragColor = col;
}