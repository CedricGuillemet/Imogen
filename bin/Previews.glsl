
//===============================================================================================
// from Iq : https://www.shadertoy.com/view/MtsGWH
vec4 boxmap( sampler2D sam, in vec3 p, in vec3 n, in float k )
{
    vec3 m = pow( abs(n), vec3(k) );
	vec4 x = texture( sam, p.yz );
	vec4 y = texture( sam, p.zx );
	vec4 z = texture( sam, p.xy );
	return (x*m.x + y*m.y + z*m.z)/(m.x+m.y+m.z);
}

vec2 envMapEquirect(vec3 wcNormal, float flipEnvMap) {
  //I assume envMap texture has been flipped the WebGL way (pixel 0,0 is a the bottom)
  //therefore we flip wcNorma.y as acos(1) = 0
  float phi = acos(-wcNormal.y);
  float theta = atan(flipEnvMap * wcNormal.x, wcNormal.z) + PI;
  return vec2(theta / TwoPI, 1.0 - phi / PI);
}

vec2 envMapEquirect(vec3 wcNormal) {
    //-1.0 for left handed coordinate system oriented texture (usual case)
    return envMapEquirect(wcNormal, -1.0);
}

float castRay( vec3 ro, vec3 rd )
{
    float delt = 0.01f;
    const float mint = 0.001f;
    const float maxt = 10.0f;
    float lh = 0.0f;
    float ly = 0.0f;
    for( float t = mint; t < maxt; t += delt )
    {
        vec3  p = ro + rd*t;
        float h = texture(Sampler0, p.xz ).x * 0.1;
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
    return 999;
}

vec4 LambertMaterial(vec2 uv, vec2 view)
{
	vec2 p = uv *vec2(2.0,-2.0) +vec2(- 1.0, 1.0);

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

    vec3 col = texture(equiRectEnvSampler, envMapEquirect(rd)).xyz;
    
	if (castRay(ro, rd)<10.0)
	{
		col = vec3(1.0);
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

        col = texture( Sampler0, 0.25*pos.xz ).xyz;
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
	return vec4( col, 1.0 );
}
