$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

uniform vec4 u_distToCenter;
uniform vec4 u_radii;
uniform vec4 u_angles;
uniform vec4 u_count;

void main()
{
	vec4 col = vec4(0.0, 0.0, 0.0, 0.0);
	for (int i=0;i<128;i++)
	{
		if (i<int(u_count.x))
		{
			float t = float(i)/(u_count.x - 0.0);
			vec2 dist = vec2(mix(u_distToCenter.x, u_distToCenter.y, t), 0.0);
			dist = Rotate2D(dist, mix(u_angles.x, u_angles.y, t));
			float radius = mix(u_radii.x, u_radii.y, t);
			float c = Circle(v_texcoord0-dist, radius, 0.0);
			col = max(col, vec4(c, c, c, c));
		}
	}
	gl_FragColor = col;
}