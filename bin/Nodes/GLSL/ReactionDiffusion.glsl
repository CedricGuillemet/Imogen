layout (std140) uniform ReactionDiffusionBlock
{
	vec2 difRate;
	vec2 feedKill;
	int PassCount;
};

vec4 getSample(vec2 uv, vec2 offset)
{
	return texture(Sampler0, uv + offset / vec2(1024.));
}

vec4 pass(vec2 uv)
{
    vec4 current = texture(Sampler0, uv);
    
    vec4 laplacian = current * -1.;
    
    laplacian += (   getSample(uv, vec2( 1., 0.) )
               + getSample(uv, vec2(-1., 0.) )
               + getSample(uv, vec2( 0., 1.) )
               + getSample(uv, vec2( 0.,-1.) )
             ) * .2;

    laplacian += (
        getSample(uv, vec2( 1., 1.) ) +
        getSample(uv, vec2( 1.,-1.) ) +
        getSample(uv, vec2(-1., 1.) ) +
        getSample(uv, vec2(-1.,-1.) ) 
       )*.05;
    
    
    float feed = 0.055;//feedKill.x;
    float kill = 0.062;//feedKill.y;
	float da = 1.;
	float db = 0.5;
	
	float a = current.x;
	float b = current.y;
	
    float ap = a + (da * laplacian.x - a * b * b + feed * (1. - a));
    float bp = b + (db * laplacian.y + a * b * b - (kill + feed) * b);
    
    //ap = clamp(ap,0.,1.);
    //bp = clamp(bp,0.,1.);
    
	//float spawn = texture(Sampler1, uv).y;
	
	//bp = max(bp, spawn);
	bp += (length(uv-vec2(0.5))<0.1)?0.01:0.0;
	
	return vec4(ap, bp, 0., 1.);
}
	
vec4 ReactionDiffusion()
{
	return pass(vUV);
}
