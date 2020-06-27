$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

SAMPLER2D(Sampler0, 0);

uniform vec4 view;

//===============================================================================================
// some code by knarkowicz https://www.shadertoy.com/view/4sSfzK
// , Iq : https://www.shadertoy.com/view/MtsGWH

float castRay( vec3 ro, vec3 rd )
{
    float delt = 0.01f;
    float mint = 0.001f;
    float maxt = 10.0f;
    float lh = 0.0f;
    float ly = 0.0f;
    for( float t = mint; t < maxt; t += delt )
    {
        vec3  p = ro + rd*t;
        float h = texture2D(Sampler0, p.xz ).x * 0.1;
        if( p.y < h )
        {
            // interpolate the intersection distance
            float resT = t - delt + delt*(lh-ly)/(p.y-ly-h+lh);
            return resT;
        }
        // allow the error to be proportinal to the distance
        delt = 0.01f*t;
        lh = h;
        ly = p.y;
    }
    return 999.;
}

void main()
{
    #if 0
	vec2 p = v_texcoord0 * 2.0 - 1.0;

     // camera movement	
	float an = view.x * PI * 2.0;
	float dn = view.y * PI * 0.5;
	float cdn = cos(dn);
	vec3 ro = vec3( 2.5*sin(an)*cdn, 1.0 + sin(dn)*2.0, 2.5*cos(an)*cdn );
    vec3 ta = vec3( 0.0, 1.0, 0.0 );
    // camera matrix
    vec3 ww = normalize( ta - ro );
    vec3 uu = normalize( cross(ww,vec3(0.0,1.0,0.0) ) );
    vec3 vv = normalize( cross(uu,ww));
	// create view ray
	vec3 rd = normalize( p.x*uu + p.y*vv + 1.5*ww );
	
    // sphere center	
	vec3 sc = vec3(0.0,1.0,0.0);

    vec3 col = texture2D(Sampler1, envMapEquirect(rd)).xyz;
    
	if (castRay(ro, rd)<10.0)
	{
		col = vec3(1.0, 1.0, 1.0);
	}
	
	/*
	// raytrace-plane
	float h = (0.0-ro.y)/rd.y;
	if( h>0.0 ) 
	{ 
		vec3 pos = ro + h*rd;
		vec3 nor = vec3(0.0,1.0,0.0); 
		vec3 di = sc - pos;
		float l = length(di);
		float occ = 1.0 - dot(nor,di/l)*1.0*1.0/(l*l); 

        col = texture2D( Sampler0, 0.25*pos.xz ).xyz;
        col *= occ;
        col *= exp(-0.1*h);
	}
	*/
/*
	// raytrace-sphere
	vec3  ce = ro - sc;
	float b = dot( rd, ce );
	float c = dot( ce, ce ) - 1.0;
	h = b*b - c;
	if( h>0.0 )
	{
		h = -b - sqrt(h);
        vec3 pos = ro + h*rd;
        vec3 nor = normalize(ro+h*rd-sc); 
        float occ = 0.5 + 0.5*nor.y;
        
        col = boxmap( Sampler0, pos, nor, 32.0 ).xyz;
        col *= occ;
    }
*/
	col = sqrt( col );
//col *= 0.1;
	gl_FragColor = vec4( col, 1.0 );
    #endif
    gl_FragColor = vec4( 1.0, 0.0, 1.0, 1.0 );
}
