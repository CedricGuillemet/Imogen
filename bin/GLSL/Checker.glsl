float Checker()
{
	vec2 nuv = vUV - vec2(0.5);
    return mod(floor(nuv.x)+floor(nuv.y),2.0);
}