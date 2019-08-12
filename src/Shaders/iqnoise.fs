$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

// Voronoise, by iq https://www.shadertoy.com/view/Xd23Dh


uniform vec4 translation;
uniform vec4 size;
uniform vec4 u;
uniform vec4 v;

void main()
{
	vec2 nuv = v_texcoord0 + translation.xy;
    vec2 p = floor(nuv*size.x);
    vec2 f = fract(nuv*size.x);
		
	float k = 1.0+63.0*pow(1.0-v.x,4.0);
	
	float va = 0.0;
	float wt = 0.0;
    for( int j=-2; j<=2; j++ )
    for( int i=-2; i<=2; i++ )
    {
        vec2 g = vec2( float(i),float(j) );
		vec3 o = hash3( p + g )*vec3(u.x,u.x,1.0);
		vec2 r = g - f + o.xy;
		float d = dot(r,r);
		float ww = pow( 1.0-smoothstep(0.0,1.414,sqrt(d)), k );
		va += o.z*ww;
		wt += ww;
    }
	
    float result = va/wt;
	gl_FragColor = vec4(result, result, result, result);
}