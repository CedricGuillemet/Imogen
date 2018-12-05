
float Box( vec3 p, vec3 b )
{
	vec3 d = abs( p ) - b;
	return min( max( d.x, max( d.y, d.z ) ), 0.0 ) + length( max( d, 0.0 ) );
}

const float maxd = 5.0;


float scene(vec3 p)
{
	float terrainHeight = texture(Sampler0, p.xz).x * 0.5;
	return max(p.y-terrainHeight, 0.0)*0.1;
}
/*
float castRay(vec3 ro, vec3 rd )
{
	float h = 0.5;
    float t = 0.0;
   
    for ( int i = 0; i < 50; ++i )
    {
		//
        if ( h < 0.0001 || t > maxd ) 
        {
            break;
        }
        
	    h = scene(ro + rd * t);
        t += h;
    }

	return t;
}
*/
float terrain(vec2 p)
{
	return texture(Sampler0, p).x * 0.5;
}
float castRay(vec3 ro, vec3 rd)
{
	float resT;
	
    float delt = 0.01f;
    const float mint = 0.001f;
    const float maxt = 4.0f;
    float lh = 0.0f;
    float ly = 0.0f;
    for( float t = mint; t < maxt; t += delt )
    {
        const vec3  p = ro + rd*t;
        const float h = terrain( p.xz );
        if( p.y < h )
        {
            // interpolate the intersection distance
            resT = t - delt + delt*(lh-ly)/(p.y-ly-h+lh);
            return resT;
        }
        // allow the error to be proportinal to the distance
        delt = 0.003f*t;
        lh = h;
        ly = p.y;
    }
    return maxd;
}


layout (std140) uniform TerrainPreviewBlock
{
	Camera camera;
} TerrainPreviewParam;

vec4 TerrainPreview()
{
	vec2 p = vUV * 2.0 - 1.0;

     // camera movement	

	vec3 ro = TerrainPreviewParam.camera.pos.xyz;//vec3( 0. );
    vec3 ta = TerrainPreviewParam.camera.pos.xyz + TerrainPreviewParam.camera.dir.xyz;
    // camera matrix
    vec3 ww = normalize( ta - ro );
    vec3 uu = normalize( cross(ww, TerrainPreviewParam.camera.up.xyz ) );
    vec3 vv = normalize( cross(uu,ww));
	// create view ray
	vec3 rd = normalize( p.x*uu + p.y*vv + 1.5*ww );
	

    vec3 col = texture(CubeSampler3, InvertCubeY(rd)).xyz + texture(Sampler0, vec2(0.)).xyz*0.01;
    
	float d = castRay(ro, rd);
	if (d<maxd)
	{
		vec3 pos = ro + rd * d;
		if (pos.x>=0 && pos.z>=0 && pos.x<=1.0 && pos.z <= 1.0)
			col = texture(Sampler1, pos.xz).xyz;
	}
	
	return vec4( col, 1.0 );
}
