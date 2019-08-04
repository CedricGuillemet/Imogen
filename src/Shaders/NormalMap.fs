$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

float u_spread;
int u_invert;

vec2 stdNormalMap(vec2 uv) 
{
    float height = texture2D(Sampler0, uv).r;
	if (u_invert != 0)
	{
		height = 1.0 - height;
	}
    return -vec2(dFdx(height), dFdy(height));
}

void main()
{
	gl_FragColor = vec4(stdNormalMap(v_texcoord0) * u_spread + 0.5, 1., 1.);
}
