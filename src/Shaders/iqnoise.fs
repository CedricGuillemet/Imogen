$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

// Voronoise, by iq https://www.shadertoy.com/view/Xd23Dh
vec3 hash3( vec2 p )
{
    vec3 q = vec3( dot(p,vec2(127.1,311.7)), 
				   dot(p,vec2(269.5,183.3)), 
				   dot(p,vec2(419.2,371.9)) );
	return fract(sin(q)*43758.5453);
}

vec2 u_translation;
float u_size;
float u_u;
float u_v;

void main()
{
	vec2 nuv = v_texcoord0 + u_translation;
    vec2 p = floor(nuv*u_size);
    vec2 f = fract(nuv*u_size);
		
	float k = 1.0+63.0*pow(1.0-u_v,4.0);
	
	float va = 0.0;
	float wt = 0.0;
    for( int j=-2; j<=2; j++ )
    for( int i=-2; i<=2; i++ )
    {
        vec2 g = vec2( float(i),float(j) );
		vec3 o = hash3( p + g )*vec3(u_u,u_u,1.0);
		vec2 r = g - f + o.xy;
		float d = dot(r,r);
		float ww = pow( 1.0-smoothstep(0.0,1.414,sqrt(d)), k );
		va += o.z*ww;
		wt += ww;
    }
	
    float result = va/wt;
	gl_FragColor = vec4(result, result, result, result);
}