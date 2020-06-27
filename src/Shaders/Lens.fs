$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

SAMPLER2D(Sampler0, 0);

uniform vec4 factor;
uniform vec4 vignette;

vec3 Distort(vec2 uv)
{
    float distCoeff = factor.x;
    uv -= 0.5;
    float r2 = dot(uv,uv);       
	float f = 1.+r2*distCoeff;
    f /= 1.+0.5*distCoeff;
    return vec3(uv*f+0.5, sqrt(r2));
}

void main()
{
	vec3 dis = Distort(v_texcoord0.xy);
	gl_FragColor = texture2D(Sampler0, dis.xy) * mix(1.0, min(1.18-dis.z, 1.0), vignette.x); 
}
