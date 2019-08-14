$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

uniform vec4 distance;
uniform vec4 radius;
uniform vec4 angle;
uniform vec4 count;

void main()
{
	vec4 col = vec4(0.0, 0.0, 0.0, 0.0);
	for (int i=0;i<128;i++)
	{
		if (i<int(count.x))
		{
			float t = float(i)/(count.x - 0.0);
			vec2 dist = vec2(mix(distance.x, distance.y, t), 0.0);
			dist = Rotate2D(dist, mix(angle.x, angle.y, t));
			float rad = mix(radius.x, radius.y, t);
			float c = Circle(v_texcoord0-dist, rad, 0.0);
			col = max(col, vec4(c, c, c, c));
		}
	}
	gl_FragColor = col;
}