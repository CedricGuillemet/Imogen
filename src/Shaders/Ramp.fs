$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

vec2 u_ramp[8];

float GetRamp(float v, vec2 arr[8]) 
{
    for (int i = 0;i<7;i++)
    {
        if (v >= arr[i].x && v <= arr[i+1].x)
        {
            // linear
            //float t = (v-arr[i].x)/(arr[i+1].x-arr[i].x);
            // smooth
            float t = smoothstep(arr[i].x, arr[i+1].x, v);
            return mix(arr[i].y, arr[i+1].y, t);
        }
    }
    
    return 0.0;
}

void main()
{
    vec4 r;
	vec4 tex = texture2D(Sampler0, v_texcoord0);
	if (u_inputIndices[0].y > -1.)
		r = texture2D(Sampler1, vec2(tex.x, 0.5));
	else
		r = tex * GetRamp(tex.x,u_ramp);

    gl_FragColor = r;
}