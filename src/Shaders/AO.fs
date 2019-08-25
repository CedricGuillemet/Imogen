$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

SAMPLER2D(Sampler0, 0);

uniform vec4 strength;// = 0.6;
uniform vec4 area;// = 0.0075;
uniform vec4 falloff;// = 0.00001;
uniform vec4 radius;// = 0.016;


float hash(vec2 p)  // replace this by something better
{
    p  = 50.0*fract( p*0.3183099 + vec2(0.71,0.113));
    return -1.0+2.0*fract( p.x*p.y*(p.x+p.y) );
}

vec3 normal_from_depth(float depth, vec2 texcoords, vec2 dx, vec2 dy)
{
  vec2 offset1 = dy;//dFdy(v_texcoord0);
  vec2 offset2 = dx;//dFdx(v_texcoord0);
  
  float depth1 = texture2D(Sampler0, texcoords + offset1).x;
  float depth2 = texture2D(Sampler0, texcoords + offset2).x;
  
  vec3 p1 = vec3(offset1, depth1 - depth);
  vec3 p2 = vec3(offset2, depth2 - depth);
  
  vec3 normal = cross(p1, p2);
  
  return normalize(normal);
}

void main()
{
	// constants

	int samples = 48;
  
	vec3 sample_sphere[48];//=vec3[48](
	sample_sphere[0] = vec3( 0.5381, 0.1856,-0.4319);
	sample_sphere[1] = vec3( 0.1379, 0.2486, 0.4430);
	sample_sphere[2] = vec3( 0.3371, 0.5679,-0.0057);
	sample_sphere[3] = vec3(-0.6999,-0.0451,-0.0019);
	sample_sphere[4] = vec3( 0.0689,-0.1598,-0.8547);
	sample_sphere[5] = vec3( 0.0560, 0.0069,-0.1843);
	sample_sphere[6] = vec3(-0.0146, 0.1402, 0.0762);
	sample_sphere[7] = vec3( 0.0100,-0.1924,-0.0344);
	sample_sphere[8] = vec3(-0.3577,-0.5301,-0.4358);
	sample_sphere[9] = vec3(-0.3169, 0.1063, 0.0158);
	sample_sphere[10] = vec3( 0.0103,-0.5869, 0.0046);
	sample_sphere[11] = vec3(-0.0897,-0.4940, 0.3287);
	sample_sphere[12] = vec3( 0.7119,-0.0154,-0.0918);
	sample_sphere[13] = vec3(-0.0533, 0.0596,-0.5411);
	sample_sphere[14] = vec3( 0.0352,-0.0631, 0.5460);
	sample_sphere[15] = vec3(-0.4776, 0.2847,-0.0271);
	sample_sphere[16] = vec3( 0.1482, -0.2644, -0.4988 );
	sample_sphere[17] = vec3( -0.1400, -0.2298, -0.4256 );
	sample_sphere[18] = vec3( -0.1811, 0.4853, -0.2447 );
	sample_sphere[19] = vec3( 0.3576, -0.3214, 0.4342 );
	sample_sphere[20] = vec3( 0.1268, -0.4914, -0.4651 );
	sample_sphere[21] = vec3( 0.3022, -0.2270, -0.0141 );
	sample_sphere[22] = vec3( 0.1402, -0.0520, -0.4085 );
	sample_sphere[23] = vec3( 0.2906, 0.1609, 0.4733 );
	sample_sphere[24] = vec3( 0.3814, -0.4596, 0.0633 );
	sample_sphere[25] = vec3( 0.4851, -0.2032, -0.4316 );
	sample_sphere[26] = vec3( -0.4744, -0.4820, -0.4254 );
	sample_sphere[27] = vec3( 0.1481, 0.0160, 0.0672 );
	sample_sphere[28] = vec3( -0.1199, -0.1149, 0.2206 );
	sample_sphere[29] = vec3( -0.2901, 0.4013, -0.4735 );
	sample_sphere[30] = vec3( 0.4643, 0.0459, 0.3613 );
	sample_sphere[31] = vec3( 0.4495, -0.1273, 0.1282 );
	sample_sphere[32] = vec3( -0.2672, 0.0208, -0.2687 );
	sample_sphere[33] = vec3( -0.0878, -0.1253, 0.3974 );
	sample_sphere[34] = vec3( -0.1237, -0.1445, 0.2443 );
	sample_sphere[35] = vec3( 0.0879, -0.4289, -0.2810 );
	sample_sphere[36] = vec3( 0.3218, -0.0686, 0.1749 );
	sample_sphere[37] = vec3( -0.2435, 0.1608, -0.4664 );
	sample_sphere[38] = vec3( -0.3292, 0.3501, 0.2427 );
	sample_sphere[39] = vec3( -0.0968, -0.0698, 0.2560 );
	sample_sphere[40] = vec3( -0.0343, 0.0098, -0.3571 );
	sample_sphere[41] = vec3( 0.3354, -0.4162, -0.0135 );
	sample_sphere[42] = vec3( 0.0088, 0.0495, 0.3811 );
	sample_sphere[43] = vec3( 0.2173, -0.1393, -0.4038 );
	sample_sphere[44] = vec3( -0.4144, -0.3722, -0.4927 );
	sample_sphere[45] = vec3( -0.1846, -0.4077, 0.3831 );
	sample_sphere[46] = vec3( -0.0111, 0.1529, 0.1266 );
	sample_sphere[47] = vec3( -0.0739, 0.2930, -0.1468 );
	
	//samples
	vec2 uvs = v_texcoord0 * 3.;
	vec3 random = normalize(vec3(hash(uvs),hash(uvs+vec2(1.0, 1.0)),hash(uvs+vec2(2.0, 2.0))));
	float depth = texture2D(Sampler0, v_texcoord0).x;
	vec3 clipSpaceNormal = normal_from_depth(depth, v_texcoord0, dFdx(v_texcoord0), dFdy(v_texcoord0));//texture2D(normalTexture, v_texcoord0).xyz *2.0 - 1.0;

	//clipSpaceNormal.z = -clipSpaceNormal.z;
	vec3 position = vec3(v_texcoord0, depth);
	// occ
	float radius_depth = radius.x/(depth+0.01);
	float occlusion = 0.0;
	for(int i=0; i < samples; i++) 
	{
		vec3 ray = radius_depth * reflect(sample_sphere[i], random);
		vec3 hemi_ray = position + sign(dot(ray,clipSpaceNormal)) * ray;
		
		float occ_depth = texture2D(Sampler0, clamp(hemi_ray.xy,0.0,1.0)).x;
		float difference = occ_depth - depth;
		
		occlusion += step(falloff.x, difference) * (1.0-smoothstep(falloff.x, area.x, difference));
	}
  
  
	float ao = 1.0 - strength.x * occlusion * (1.0 / float(samples));
	
	//return vec4(clipSpaceNormal*0.5+0.5, 1.0);
	gl_FragColor = vec4(ao, ao, ao, ao);
}
