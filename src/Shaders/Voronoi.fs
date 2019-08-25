$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

SAMPLER2D(Sampler0, 0);

uniform vec4 translation;
uniform vec4 size;
uniform vec4 noise;
uniform vec4 colorInterpolation;
uniform vec4 distanceBlend;


float rand(float n){return fract(sin(n) * 43758.5453123);}

float sdAxisAlignedRect(vec2 uv, vec2 tl, vec2 br)
{
    vec2 d = max(tl-uv, uv-br);
    return length(max(vec2(0.0, 0.0), d)) + min(0.0, max(d.x, d.y));
}

float sdAxisAlignedRectManhattan(vec2 uv, vec2 tl, vec2 br)
{
    vec2 d = max(tl-uv, uv-br);
    vec2 dif = vec2(max(vec2(0.0, 0.0), d)) + min(0.0, max(d.x, d.y));
    return max(dif.x, dif.y);
}

void main()
{
    
	vec2 nuv = v_texcoord0 + translation.xy;
    //vec2 pst = floor(v_texcoord0*size.x);
    vec2 p = floor(nuv*size.x);
    vec2 f = fract(nuv*size.x);
		
	float k = 1.0+63.0*pow(1.0-colorInterpolation.x,4.0);
	
	vec3 va = vec3(0.,0.,0.);
	float wt = 0.0;
    vec3 color;
    for( int j=-2; j<=2; j++ )
    {
        for( int i=-2; i<=2; i++ )
        {
            vec2 g = vec2( float(i),float(j) );
            vec3 o = hash3( p + g )*vec3(noise.x,noise.x,1.0);
                
            vec2 r = g - f + o.xy;
            float d = mix(sqrt(dot(r,r)), abs(r.x)+abs(r.y), distanceBlend.x);
            float ww = pow( 1.0-smoothstep(0.0,1.414,d), k );

            if (u_inputIndices[0].x > -1.)
            {
                color = texture2D(Sampler0, (p + g + o.xy ) / size.x - translation.xy).xyz;
            }
            else
            {
                color = vec3(o.zzz);
            }
            va += color * ww;
            wt += ww;
        }
    }

    vec3 result = va/wt;
	gl_FragColor = vec4(result, 1.);
}