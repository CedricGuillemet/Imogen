/////////////////////////////////////////////////////////////////////////
// PBR by knarkowicz https://www.shadertoy.com/view/4sSfzK

layout (std140) uniform PBRBlock
{
	vec2 view;
	float displacementFactor;
	int geometry;
} PBRParam;

float camHeight = -0.2;

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

float displace(vec3 p)
{
	float disp = boxmap(Sampler3, p, normalize(p), 1.0 ).x;
	return -disp * PBRParam.displacementFactor;
}

float DoorKnob( vec3 p, mat3 localToWorld )
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
	return ret;
}

float Scene( vec3 p, mat3 localToWorld )
{
	vec3 centerMesh = vec3(0., -camHeight, 0.);
	float ret = 0.;
	
    switch(PBRParam.geometry)
	{
	case 0:
		ret = DoorKnob(p, localToWorld);
		break;
	case 1:
		ret = Sphere(p+centerMesh, 1.0);
		break;
	case 2:
		ret = Box(p+centerMesh, vec3(0.5));
		break;
	case 3:
		ret = max(p.y, 0.);
		break;
	case 4:
		ret = Cylinder(p+centerMesh, 0.7, 0.7);
		break;
	}
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

vec4 PBR()
{
	vec2 p = vUV * 2.0 - 1.0;

     // camera movement	
	float an = PBRParam.view.x * PI * 2.0;
	float dn = PBRParam.view.y * PI * 0.5;
	float cdn = cos(dn);

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

    vec3 col = texture(CubeSampler4, InvertCubeY(rd)).xyz;
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

        vec3 env       = textureLod(Sampler4, envMapEquirect(refl),roughnessE*12.0).xyz;
        
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