
layout (std140) uniform CubeRadianceBlock
{
	int size;
};

/*
 * Copyright LWJGL. All rights reserved.
 * License terms: https://www.lwjgl.org/license
 */

#define PI 3.14159265359

/**
 * Generate a uniformly distributed random point on the unit disk oriented around 'n'.
 * 
 * After:
 * http://mathworld.wolfram.com/DiskPointPicking.html
 */
vec3 randomDiskPoint(vec3 rand, vec3 n) {
  float r = rand.x * 0.5 + 0.5; // [-1..1) -> [0..1)
  float angle = (rand.y + 1.0) * PI; // [-1..1] -> [0..2*PI)
  float sr = sqrt(r);
  vec2 p = vec2(sr * cos(angle), sr * sin(angle));
  /*
   * Compute some arbitrary tangent space for orienting
   * our disk towards the normal. We use the camera's up vector
   * to have some fix reference vector over the whole screen.
   */
  vec3 tangent = normalize(rand);
  vec3 bitangent = cross(tangent, n);
  tangent = cross(bitangent, n);
  
  /* Make our disk orient towards the normal. */
  return tangent * p.x + bitangent * p.y;
}

/**
 * Generate a uniformly distributed random point on the unit-sphere.
 * 
 * After:
 * http://mathworld.wolfram.com/SpherePointPicking.html
 */
vec3 randomSpherePoint(vec3 rand) {
  float ang1 = (rand.x + 1.0) * PI; // [-1..1) -> [0..2*PI)
  float u = rand.y; // [-1..1), cos and acos(2v-1) cancel each other out, so we arrive at [-1..1)
  float u2 = u * u;
  float sqrt1MinusU2 = sqrt(1.0 - u2);
  float x = sqrt1MinusU2 * cos(ang1);
  float y = sqrt1MinusU2 * sin(ang1);
  float z = u;
  return vec3(x, y, z);
}

/**
 * Generate a uniformly distributed random point on the unit-hemisphere
 * around the given normal vector.
 * 
 * This function can be used to generate reflected rays for diffuse surfaces.
 * Actually, this function can be used to sample reflected rays for ANY surface
 * with an arbitrary BRDF correctly.
 * This is because we always need to solve the integral over the hemisphere of
 * a surface point by using numerical approximation using a sum of many
 * sample directions.
 * It is only with non-lambertian BRDF's that, in theory, we could sample them more
 * efficiently, if we knew in which direction the BRDF reflects the most energy.
 * This would be importance sampling, but care must be taken as to not over-estimate
 * those surfaces, because then our sum for the integral would be greater than the
 * integral itself. This is the inherent problem with importance sampling.
 * 
 * The points are uniform over the sphere and NOT over the projected disk
 * of the sphere, so this function cannot be used when sampling a spherical
 * light, where we need to sample the projected surface of the light (i.e. disk)!
 */
vec3 randomHemispherePoint(vec3 rand, vec3 n) {
  /**
   * Generate random sphere point and swap vector along the normal, if it
   * points to the wrong of the two hemispheres.
   * This method provides a uniform distribution over the hemisphere, 
   * provided that the sphere distribution is also uniform.
   */
  vec3 v = randomSpherePoint(rand);
  return v * sign(dot(v, n));
}

vec3 ortho(vec3 v) {
    //  See : http://lolengine.net/blog/2013/09/21/picking-orthogonal-vector-combing-coconuts
    return abs(v.x) > abs(v.z) ? vec3(-v.y, v.x, 0.0)  : vec3(0.0, -v.z, v.y);
}

/**
 * Generate a cosine-weighted random point on the unit hemisphere oriented around 'n'.
 * 
 * @param rand a vector containing pseudo-random values
 * @param n    the normal to orient the hemisphere around
 * @returns    the cosine-weighted point on the oriented hemisphere
 */
vec3 randomCosineWeightedHemispherePoint(vec3 rand, vec3 n) {
  float r = rand.x * 0.5 + 0.5; // [-1..1) -> [0..1)
  float angle = (rand.y + 1.0) * PI; // [-1..1] -> [0..2*PI)
  float sr = sqrt(r);
  vec2 p = vec2(sr * cos(angle), sr * sin(angle));
  /*
   * Unproject disk point up onto hemisphere:
   * 1.0 == sqrt(x*x + y*y + z*z) -> z = sqrt(1.0 - x*x - y*y)
   */
  vec3 ph = vec3(p.xy, sqrt(1.0 - p*p));
  /*
   * Compute some arbitrary tangent space for orienting
   * our hemisphere 'ph' around the normal. We use the camera's up vector
   * to have some fix reference vector over the whole screen.
   */
  vec3 tangent = normalize(rand);
  vec3 bitangent = cross(tangent, n);
  tangent = cross(bitangent, n);
  
  /* Make our hemisphere orient around the normal. */
  return tangent * ph.x + bitangent * ph.y + n * ph.z;
}


uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

// Compound versions of the hashing algorithm I whipped together.
uint hash( uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
uint hash( uvec3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
uint hash( uvec4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}

// Pseudo-random value in half-open range [0:1].
float random( float x ) { return floatConstruct(hash(floatBitsToUint(x))); }
float random( vec2  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec3  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec4  v ) { return floatConstruct(hash(floatBitsToUint(v))); }


vec3 get_world_normal()
{
	vec3 dir = (EvaluationParam.viewRot * vec4(vUV * 2.0 - 1.0, 1.0, 0.0)).xyz;
	return normalize(dir);
}

vec4 CubeRadiance()
{
	vec4 res = vec4(0.0);
	vec3 n = get_world_normal();
	n.y = -n.y;
	
	for(int i = 0;i<400;i++)
	{
		float iv = float(i);
		vec3 rand = vec3(random(vec3(gl_FragCoord.xy, iv)), random(vec3(gl_FragCoord.yx, iv)), random(vec2(gl_FragCoord.x * gl_FragCoord.y, iv)));
		res += textureLod(CubeSampler0, randomCosineWeightedHemispherePoint(rand, n), 0.0);
	}
	res /= 400.;
	return res;
}
