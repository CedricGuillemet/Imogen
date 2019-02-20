layout (std140) uniform ReactionDiffusionBlock
{
	vec2 difRate;
	vec2 feedKill;
	int PassCount;
};




/*vec2 difRate = vec2(1.,.25);

#define FEED .0367;
#define KILL .0649;
*/

//float zoom = .9997;

vec4 getSample(vec2 uv, vec2 offset)
{
	return texture(Sampler0, uv + offset * vec2(0.001, 0.001)/*EvaluationParam.viewport*/);
}

vec4 pass(vec2 uv)
{
	//uv =( uv - vec2(.5)) * zoom + vec2(.5);

    vec4 current = getSample(uv, vec2(0.));
    
    vec4 cumul = current * -1.;
    
    cumul += (   getSample(uv, vec2( 1., 0.) )
               + getSample(uv, vec2(-1., 0.) )
               + getSample(uv, vec2( 0., 1.) )
               + getSample(uv, vec2( 0.,-1.) )
             ) * .2;

    cumul += (
        getSample(uv, vec2( 1., 1.) ) +
        getSample(uv, vec2( 1.,-1.) ) +
        getSample(uv, vec2(-1., 1.) ) +
        getSample(uv, vec2(-1.,-1.) ) 
       )*.05;
    
    
    float feed = feedKill.x;
    float kill = feedKill.y;
   
    vec4 lap =  cumul;
    float newR = current.r + (difRate.r * lap.r - current.r * current.g * current.g + feed * (1. - current.r));
    float newG = current.g + (difRate.g * lap.g + current.r * current.g * current.g - (kill + feed) * current.g);
    
    newR = clamp(newR,0.,1.);
    newG = clamp(newG,0.,1.);
    
	current = vec4(newR,newG,0.,1.);
	
	    	uv -= 0.5;
    	float f = step(length(uv),.25) - step(length(uv),.24);
    	f *=  .25 + fract(atan(uv.y,uv.x)*.5) * .25;// * sin(iTime*.1);
        current = max(current, vec4(0.,1.,0.,1.) * f);
	
    return current;
}
	
vec4 ReactionDiffusion()
{
	return pass(vUV);
}
