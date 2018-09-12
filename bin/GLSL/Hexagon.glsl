float Hexagon()
{
	vec2 V = vec2(.866,.5);
    vec2 v = abs ( ((vUV * 2.0)-1.0) * mat2( V, -V.y, V.x) );	
	return ceil( 1. - max(v.y, dot( v, V)) *1.15  );
}
