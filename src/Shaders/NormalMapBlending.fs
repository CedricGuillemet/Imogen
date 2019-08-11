$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

////////////////////////////////////////////////////////////////////////////////////////
// Normal map blending by ZigguratVertigo https://www.shadertoy.com/view/4t2SzR

uniform vec4 u_technique;

#define TECHNIQUE_RNM 				 0
#define TECHNIQUE_PartialDerivatives 1
#define TECHNIQUE_Whiteout 			 2
#define TECHNIQUE_UDN				 3
#define TECHNIQUE_Unity				 4
#define TECHNIQUE_Linear		     5
#define TECHNIQUE_Overlay		     6

// RNM
vec3 NormalBlend_RNM(vec3 n1, vec3 n2)
{
    // Unpack (see article on why it's not just n*2-1)
	n1 = n1*vec3( 2,  2, 2) + vec3(-1, -1,  0);
    n2 = n2*vec3(-2, -2, 2) + vec3( 1,  1, -1);
    
    // Blend
    return n1*dot(n1, n2)/n1.z - n2;
}

// RNM - Already unpacked
vec3 NormalBlend_UnpackedRNM(vec3 n1, vec3 n2)
{
	n1 += vec3(0, 0, 1);
	n2 *= vec3(-1, -1, 1);
	
    return n1*dot(n1, n2)/n1.z - n2;
}

// Partial Derivatives
vec3 NormalBlend_PartialDerivatives(vec3 n1, vec3 n2)
{	
    // Unpack
	n1 = n1*2.0 - 1.0;
    n2 = n2*2.0 - 1.0;
    
    return normalize(vec3(n1.xy*n2.z + n2.xy*n1.z, n1.z*n2.z));
}

// Whiteout
vec3 NormalBlend_Whiteout(vec3 n1, vec3 n2)
{
    // Unpack
	n1 = n1*2.0 - 1.0;
    n2 = n2*2.0 - 1.0;
    
	return normalize(vec3(n1.xy + n2.xy, n1.z*n2.z));    
}

// UDN
vec3 NormalBlend_UDN(vec3 n1, vec3 n2)
{
    // Unpack
	n1 = n1*2.0 - 1.0;
    n2 = n2*2.0 - 1.0;    
    
	return normalize(vec3(n1.xy + n2.xy, n1.z));
}

// Unity
vec3 NormalBlend_Unity(vec3 n1, vec3 n2)
{
    // Unpack
	n1 = n1*2.0 - 1.0;
    n2 = n2*2.0 - 1.0;
    
    mat3 nBasis = mat3(vec3(n1.z, n1.y, -n1.x), // +90 degree rotation around y axis
        			   vec3(n1.x, n1.z, -n1.y), // -90 degree rotation around x axis
        			   vec3(n1.x, n1.y,  n1.z));
	
    return normalize(n2.x*nBasis[0] + n2.y*nBasis[1] + n2.z*nBasis[2]);
}

// Linear Blending
vec3 NormalBlend_Linear(vec3 n1, vec3 n2)
{
    // Unpack
	n1 = n1*2.0 - 1.0;
    n2 = n2*2.0 - 1.0;
    
	return normalize(n1 + n2);    
}

// Overlay
float overlay(float x, float y)
{
    if (x < 0.5)
        return 2.0*x*y;
    else
        return 1.0 - 2.0*(1.0 - x)*(1.0 - y);
}
vec3 NormalBlend_Overlay(vec3 n1, vec3 n2)
{
    vec3 n;
    n.x = overlay(n1.x, n2.x);
    n.y = overlay(n1.y, n2.y);
    n.z = overlay(n1.z, n2.z);

    return normalize(n*2.0 - 1.0);
}

// Combine normals
vec3 CombineNormal(vec3 n1, vec3 n2)
{
    int tech = int(u_technique.x);
 	if (tech == TECHNIQUE_RNM)
        return NormalBlend_RNM(n1, n2);
    else if (tech == TECHNIQUE_PartialDerivatives)
        return NormalBlend_PartialDerivatives(n1, n2);
    else if (tech == TECHNIQUE_Whiteout)
        return NormalBlend_Whiteout(n1, n2);
    else if (tech == TECHNIQUE_UDN)
        return NormalBlend_UDN(n1, n2);
    else if (tech == TECHNIQUE_Unity)
        return NormalBlend_Unity(n1, n2);
    else if (tech == TECHNIQUE_Linear)
        return NormalBlend_Linear(n1, n2);
    else
        return NormalBlend_Overlay(n1, n2);
}

void main()
{
	vec3 n1 = texture2D(Sampler0, v_texcoord0).xyz;
	vec3 n2 = texture2D(Sampler1, v_texcoord0).xyz;
	gl_FragColor = vec4(CombineNormal(n1, n2) * 0.5 + 0.5, 1.0);
}
