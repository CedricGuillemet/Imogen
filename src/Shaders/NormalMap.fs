$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

uniform vec4 u_spread;
uniform vec4 u_invert;

vec2 stdNormalMap(vec2 uv) 
{
    float height = texture2D(Sampler0, uv).r;
	if (u_invert.x > 0.001)
	{
		height = 1.0 - height;
	}
    return -vec2(dFdx(height), dFdy(height));
}

void main()
{
	gl_FragColor = vec4(stdNormalMap(v_texcoord0) * u_spread.x + 0.5, 1., 1.);
}
