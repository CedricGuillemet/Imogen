$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

uniform vec4 gradient[8];

vec4 GetRamp(float v, vec4 arr[8]) 
{
    for (int i = 0;i<7;i++)
    {
        if (v >= arr[i].w && v <= arr[i+1].w)
        {
            // linear
            float t = (v-arr[i].w)/(arr[i+1].w-arr[i].w);
            // smooth
            //float t = smoothstep(arr[i].w, arr[i+1].w, v);
            return sqrt(mix(arr[i]*arr[i], arr[i+1]*arr[i+1], t));
        }
    }
    
    return vec4(0.0, 0.0, 0.0, 0.0);
}

void main()
{
	gl_FragColor = vec4(GetRamp(v_texcoord0.x, gradient).xyz, 1.0);
}