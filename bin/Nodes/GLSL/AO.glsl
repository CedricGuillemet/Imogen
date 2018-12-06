layout (std140) uniform AOBlock
{
	float total_strength;// = 0.6;

	float area;// = 0.0075;
	float falloff;// = 0.00001;

	float radius;// = 0.016;
} AOParam;


float hash(vec2 p)  // replace this by something better
{
    p  = 50.0*fract( p*0.3183099 + vec2(0.71,0.113));
    return -1.0+2.0*fract( p.x*p.y*(p.x+p.y) );
}

vec3 normal_from_depth(float depth, vec2 texcoords) {
  
  const vec2 offset1 = dFdy(vUV);
  const vec2 offset2 = dFdx(vUV);
  
  float depth1 = texture(Sampler0, texcoords + offset1).x;
  float depth2 = texture(Sampler0, texcoords + offset2).x;
  
  vec3 p1 = vec3(offset1, depth1 - depth);
  vec3 p2 = vec3(offset2, depth2 - depth);
  
  vec3 normal = cross(p1, p2);
  
  return normalize(normal);
}


float AO()
{
	// constants

	const int samples = 48;
  
	const vec3 sample_sphere[48]=vec3[48](
	vec3( 0.5381, 0.1856,-0.4319), vec3( 0.1379, 0.2486, 0.4430),
	vec3( 0.3371, 0.5679,-0.0057), vec3(-0.6999,-0.0451,-0.0019),
	vec3( 0.0689,-0.1598,-0.8547), vec3( 0.0560, 0.0069,-0.1843),
	vec3(-0.0146, 0.1402, 0.0762), vec3( 0.0100,-0.1924,-0.0344),
	vec3(-0.3577,-0.5301,-0.4358), vec3(-0.3169, 0.1063, 0.0158),
	vec3( 0.0103,-0.5869, 0.0046), vec3(-0.0897,-0.4940, 0.3287),
	vec3( 0.7119,-0.0154,-0.0918), vec3(-0.0533, 0.0596,-0.5411),
	vec3( 0.0352,-0.0631, 0.5460), vec3(-0.4776, 0.2847,-0.0271),
	vec3( 0.1482, -0.2644, -0.4988 ),vec3( -0.1400, -0.2298, -0.4256 ),
	vec3( -0.1811, 0.4853, -0.2447 ),vec3( 0.3576, -0.3214, 0.4342 ),
	vec3( 0.1268, -0.4914, -0.4651 ),vec3( 0.3022, -0.2270, -0.0141 ),
	vec3( 0.1402, -0.0520, -0.4085 ),vec3( 0.2906, 0.1609, 0.4733 ),
	vec3( 0.3814, -0.4596, 0.0633 ),vec3( 0.4851, -0.2032, -0.4316 ),
	vec3( -0.4744, -0.4820, -0.4254 ),vec3( 0.1481, 0.0160, 0.0672 ),
	vec3( -0.1199, -0.1149, 0.2206 ),vec3( -0.2901, 0.4013, -0.4735 ),
	vec3( 0.4643, 0.0459, 0.3613 ),vec3( 0.4495, -0.1273, 0.1282 ),
	vec3( -0.2672, 0.0208, -0.2687 ),vec3( -0.0878, -0.1253, 0.3974 ),
	vec3( -0.1237, -0.1445, 0.2443 ),vec3( 0.0879, -0.4289, -0.2810 ),
	vec3( 0.3218, -0.0686, 0.1749 ),vec3( -0.2435, 0.1608, -0.4664 ),
	vec3( -0.3292, 0.3501, 0.2427 ),vec3( -0.0968, -0.0698, 0.2560 ),
	vec3( -0.0343, 0.0098, -0.3571 ),vec3( 0.3354, -0.4162, -0.0135 ),
	vec3( 0.0088, 0.0495, 0.3811 ),vec3( 0.2173, -0.1393, -0.4038 ),
	vec3( -0.4144, -0.3722, -0.4927 ),vec3( -0.1846, -0.4077, 0.3831 ),
	vec3( -0.0111, 0.1529, 0.1266 ),vec3( -0.0739, 0.2930, -0.1468 )
	);
	
	
	//samples
	vec2 uvs = vUV * 3.;
	vec3 random = normalize(vec3(hash(uvs),hash(uvs+vec2(1.0)),hash(uvs+vec2(2.0))));
	float depth = texture(Sampler0, vUV).x;
	vec3 clipSpaceNormal = normal_from_depth(depth, vUV);//texture2D(normalTexture, vUV).xyz *2.0 - 1.0;

	//clipSpaceNormal.z = -clipSpaceNormal.z;
	vec3 position = vec3(vUV, depth);
	// occ
	float radius_depth = AOParam.radius/(depth+0.01);
	float occlusion = 0.0;
	for(int i=0; i < samples; i++) 
	{
		vec3 ray = radius_depth * reflect(sample_sphere[i], random);
		vec3 hemi_ray = position + sign(dot(ray,clipSpaceNormal)) * ray;
		
		float occ_depth = texture(Sampler0, clamp(hemi_ray.xy,0.0,1.0)).x;
		float difference = occ_depth - depth;
		
		occlusion += step(AOParam.falloff, difference) * (1.0-smoothstep(AOParam.falloff, AOParam.area, difference));
	}
  
  
	float ao = 1.0 - AOParam.total_strength * occlusion * (1.0 / samples);
	
	//return vec4(clipSpaceNormal*0.5+0.5, 1.0);
	return ao;
}
