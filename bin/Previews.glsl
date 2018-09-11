
//===============================================================================================
// some code by knarkowicz https://www.shadertoy.com/view/4sSfzK
// , Iq : https://www.shadertoy.com/view/MtsGWH

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

vec4 LambertMaterial(vec2 view)
{
	vec2 p = vUV *vec2(2.0,-2.0) +vec2(- 1.0, 1.0);

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
//col *= 0.1;
	return vec4( col, 1.0 );
}

/////////////////////////////////////////////////////////////////////////
// PBR by knarkowicz https://www.shadertoy.com/view/4sSfzK

float saturate( float x )
{
    return clamp( x, 0., 1. );
}

vec3 saturate( vec3 x )
{
    return clamp( x, vec3( 0. ), vec3( 1. ) );
}

float Smooth( float x )
{
	return smoothstep( 0., 1., saturate( x ) );   
}

void Rotate( inout vec2 p, float a ) 
{
    p = cos( a ) * p + sin( a ) * vec2( p.y, -p.x );
}

float RaySphere( vec3 rayOrigin, vec3 rayDir, vec3 spherePos, float sphereRadius )
{
	vec3 oc = rayOrigin - spherePos;
	
	float b = dot( oc, rayDir );
	float c = dot( oc, oc ) - sphereRadius * sphereRadius;
	float h = b * b - c;
	
	float t;
	if ( h < 0.0 )
    {
		t = -1.0;
    }
	else
    {
		t = ( -b - sqrt( h ) );
    }
	return t;
}

float VisibilityTerm( float roughness, float ndotv, float ndotl )
{
	float r2 = roughness * roughness;
	float gv = ndotl * sqrt( ndotv * ( ndotv - ndotv * r2 ) + r2 );
	float gl = ndotv * sqrt( ndotl * ( ndotl - ndotl * r2 ) + r2 );
	return 0.5 / max( gv + gl, 0.00001 );
}

float DistributionTerm( float roughness, float ndoth )
{
	float r2 = roughness * roughness;
	float d	 = ( ndoth * r2 - ndoth ) * ndoth + 1.0;
	return r2 / ( d * d * PI );
}

vec3 FresnelTerm( vec3 specularColor, float vdoth )
{
	vec3 fresnel = specularColor + ( 1. - specularColor ) * pow( ( 1. - vdoth ), 5. );
	return fresnel;
}

vec3 EnvBRDFApprox( vec3 specularColor, float roughness, float ndotv )
{
    const vec4 c0 = vec4( -1, -0.0275, -0.572, 0.022 );
    const vec4 c1 = vec4( 1, 0.0425, 1.04, -0.04 );
    vec4 r = roughness * c0 + c1;
    float a004 = min( r.x * r.x, exp2( -9.28 * ndotv ) ) * r.x + r.y;
    vec2 AB = vec2( -1.04, 1.04 ) * a004 + r.zw;
    return specularColor * AB.x + AB.y;  
}

vec3 EnvRemap( vec3 c )
{
    return pow( 2. * c, vec3( 2.2 ) );
}

float Cylinder( vec3 p, float r, float height ) 
{
	float d = length( p.xz ) - r;
	d = max( d, abs( p.y ) - height );
	return d;
}

float Substract( float a, float b )
{
    return max( a, -b );
}

float SubstractRound( float a, float b, float r ) 
{
	vec2 u = max( vec2( r + a, r - b ), vec2( 0.0, 0.0 ) );
	return min( -r, max( a, -b ) ) + length( u );
}

float Union( float a, float b )
{
    return min( a, b );
}

float Box( vec3 p, vec3 b )
{
	vec3 d = abs( p ) - b;
	return min( max( d.x, max( d.y, d.z ) ), 0.0 ) + length( max( d, 0.0 ) );
}

float Sphere( vec3 p, float s )
{
	return length( p ) - s;
}

float Torus( vec3 p, float sr, float lr )
{
	return length( vec2( length( p.xz ) - lr, p.y ) ) - sr;
}

float Disc( vec3 p, float r, float t ) 
{
	float l = length( p.xz ) - r;
	return l < 0. ? abs( p.y ) - t : length( vec2( p.y, l ) ) - t;
}

float UnionRound( float a, float b, float k )
{
    float h = clamp( 0.5 + 0.5 * ( b - a ) / k, 0.0, 1.0 );
    return mix( b, a, h ) - k * h * ( 1.0 - h );
}

float displace(vec3 p)
{
	float disp = boxmap(Sampler3, p, normalize(p), 1.0 ).x;
//float disp = texture
	return -disp * 0.01;
}
float Scene( vec3 p, mat3 localToWorld )
{
    p = p * localToWorld;
    
    // ring
    vec3 t = p;
    t.y -= -.7;
    float r = Substract( Disc( t, 0.9, .1 ), Cylinder( t, .7, 2. ) );
    vec3 t2 = t - vec3( 0., 0., 1.0 );
    Rotate( t2.xz, 0.25 * PI );
    r = Substract( r, Box( t2, vec3( .5 ) ) );
    r = Union( r, Disc( t + vec3( 0., 0.05, 0. ), 0.85, .05 ) );
    
    t = p;
    Rotate( t.yz, -.3 );
    
    // body
    float b = Sphere( t, .8 );
    b = Substract( b, Sphere( t - vec3( 0., 0., .5 ), .5 ) );
    b = Substract( b, Sphere( t - vec3( 0., 0., -.7 ), .3 ) );
    b = Substract( b, Box( t, vec3( 2., .03, 2. ) ) );
    b = Union( b, Sphere( t, .7 ) );

    float ret = Union( r, b );
	ret += displace(p);
	return ret;
}

float CastRay( in vec3 ro, in vec3 rd, mat3 localToWorld )
{
    const float maxd = 5.0;
    
	float h = 0.5;
    float t = 0.0;
   
    for ( int i = 0; i < 50; ++i )
    {
        if ( h < 0.001 || t > maxd ) 
        {
            break;
        }
        
	    h = Scene( ro + rd * t, localToWorld );
        t += h;
    }

    if ( t > maxd )
    {
        t = -1.0;
    }
	
    return t;
}

vec3 SceneNormal( in vec3 pos, mat3 localToWorld )
{
	vec3 eps = vec3( 0.001, 0.0, 0.0 );
	vec3 nor = vec3(
	    Scene( pos + eps.xyy, localToWorld ) - Scene( pos - eps.xyy, localToWorld ),
	    Scene( pos + eps.yxy, localToWorld ) - Scene( pos - eps.yxy, localToWorld ),
	    Scene( pos + eps.yyx, localToWorld ) - Scene( pos - eps.yyx, localToWorld ) );
	return normalize( nor );
}

float SceneAO( vec3 p, vec3 n, mat3 localToWorld )
{
    float ao = 0.0;
    float s = 1.0;
    for( int i = 0; i < 6; ++i )
    {
        float off = 0.001 + 0.2 * float( i ) / 5.;
        float t = Scene( n * off + p, localToWorld );
        ao += ( off - t ) * s;
        s *= 0.4;
    }
    
    return Smooth( 1.0 - 12.0 * ao );
}


vec4 PBR(vec2 view)
{
	vec2 p = vUV *vec2(2.0,-2.0) +vec2(- 1.0, 1.0);

     // camera movement	
	float an = view.x * PI * 2.0;
	float dn = view.y * PI * 0.5;
	float cdn = cos(dn);
float camHeight = -0.2;
	vec3 ro = vec3( 2.0*sin(an)*cdn, camHeight + sin(dn)*2.0, 2.0*cos(an)*cdn );
    vec3 ta = vec3( 0.0, camHeight, 0.0 );
    // camera matrix
    vec3 ww = normalize( ta - ro );
    vec3 uu = normalize( cross(ww,vec3(0.0,1.0,0.0) ) );
    vec3 vv = normalize( cross(uu,ww));
	// create view ray
	vec3 rd = normalize( p.x*uu + p.y*vv + 1.5*ww );
	
    // sphere center	
	vec3 sc = vec3(0.0,1.0,0.0);
	mat3 localToWorld = mat3(vec3(1.0,0.0,0.0), vec3(0.0,1.0,0.0), vec3(0.0,0.0,1.0));


    vec3 lightColor = vec3( 2. );
    vec3 lightDir = normalize( vec3( .7, .9, -.2 ) );

    vec3 col = texture(equiRectEnvSampler, envMapEquirect(rd)).xyz;
	float t = CastRay( ro, rd, localToWorld );
    if ( t > 0.0 )
    {
        vec3 pos = ro + t * rd;
        vec3 normal = SceneNormal( pos, localToWorld );        
        vec3 viewDir = -rd;
        vec3 refl = reflect( rd, normal );
        
        vec3 diffuse  = vec3( 0. );
        vec3 specular = vec3( 0. );
        
		vec3 baseColor     = pow(boxmap(Sampler0, pos, normalize(pos), 1.0 ).xyz, vec3( 2.2 ) );

		float roughness = boxmap(Sampler2, pos, normalize(pos), 1.0 ).x;
	    vec3 diffuseColor  = vec3( 0.1 );// : baseColor;
 	    vec3 specularColor = baseColor;// : vec3( 0.02 );
   		float roughnessE   = roughness * roughness;
	    float roughnessL   = max( .01, roughnessE );

        vec3 halfVec = normalize( viewDir + lightDir );
        float vdoth = saturate( dot( viewDir, halfVec ) );
        float ndoth	= saturate( dot( normal, halfVec ) );
        float ndotv = saturate( dot( normal, viewDir ) );
        float ndotl = saturate( dot( normal, lightDir ) );

        vec3 envSpecularColor = EnvBRDFApprox( specularColor, roughnessE, ndotv );

        vec3 env       = textureLod(equiRectEnvSampler, envMapEquirect(refl),roughnessE*12.0).xyz;
        
        diffuse += diffuseColor * EnvRemap(env);
        specular += envSpecularColor * env;

		diffuse  += diffuseColor * saturate( dot( normal, lightDir ) );
            
   		vec3 lightF = FresnelTerm( specularColor, vdoth );
		float lightD = DistributionTerm( roughnessL, ndoth );
		float lightV = VisibilityTerm( roughnessL, ndotv, ndotl );
        specular += lightColor * lightF * ( lightD * lightV * PI * ndotl );
        
        float ao = SceneAO( pos, normal, localToWorld );
        diffuse *= ao;
        specular *= saturate( pow( ndotv + ao, roughnessE ) - 1. + ao);
        
        col = diffuse + specular;

    }
    col = pow( col * .4, vec3( 1. / 2.2 ) );
	return vec4(col,1.0);
}