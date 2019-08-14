
mat3 cotangent_frame( vec3 N, vec3 p, vec2 uv )
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );

    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, N );
}

float GetHeight(sampler2D heightSampler, vec2 texCoords)
{
    return texture2D(heightSampler, texCoords).r;
}

//Parallax Occlusion Mapping from: https://learnopengl.com/Advanced-Lighting/Parallax-Mapping
vec2 ParallaxMapping(sampler2D heightSampler, vec2 texCoords, vec3 viewDir, float depthFactor)
{ 
    float height_scale = depthFactor;
    // number of depth layers
    float minLayers = 8.0;
    float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, min(abs(viewDir.z), 1.0));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy * height_scale; 
    vec2 deltaTexCoords = P / numLayers; 

    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = GetHeight(heightSampler, currentTexCoords);
  
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = GetHeight(heightSampler, currentTexCoords);  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }

    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = GetHeight(heightSampler, prevTexCoords) + layerDepth - currentLayerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = mix(currentTexCoords, prevTexCoords, weight);

    return finalTexCoords;  
} 

vec3 hash3( vec2 p )
{
    vec3 q = vec3( dot(p,vec2(127.1,311.7)), 
				   dot(p,vec2(269.5,183.3)), 
				   dot(p,vec2(419.2,371.9)) );
	return fract(sin(q)*43758.5453);
}

/*
vec4 cubemap2D( sampler2D sam, in vec3 d )
{
    vec3 n = abs(d);
    vec3 s = dFdx(d);
    vec3 t = dFdy(d);
         if(n.x>n.y && n.x>n.z) {d=d.xyz;s=s.xyz;t=t.xyz;}
    else if(n.y>n.x && n.y>n.z) {d=d.yzx;s=s.yzx;t=t.yzx;}
    else                        {d=d.zxy;s=s.zxy;t=t.zxy;}
    vec2 q = d.yz/d.x;
    return texture2Dgrad( sam, 
                        0.5*q + 0.5,
                        0.5*(s.yz-q*s.x)/d.x,
                        0.5*(t.yz-q*t.x)/d.x );
}
*/

SAMPLER2D(Sampler0, 0);
SAMPLER2D(Sampler1, 1);
SAMPLER2D(Sampler2, 2);
SAMPLER2D(Sampler3, 3);
SAMPLER2D(Sampler4, 4);
SAMPLER2D(Sampler5, 5);
SAMPLER2D(Sampler6, 6);
SAMPLER2D(Sampler7, 7);
SAMPLERCUBE(CubeSampler0, 8);
SAMPLERCUBE(CubeSampler1, 9);
SAMPLERCUBE(CubeSampler2, 10);
SAMPLERCUBE(CubeSampler3, 11);
SAMPLERCUBE(CubeSampler4, 12);
SAMPLERCUBE(CubeSampler5, 13);
SAMPLERCUBE(CubeSampler6, 14);
SAMPLERCUBE(CubeSampler7, 15);

