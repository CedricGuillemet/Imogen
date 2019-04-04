layout (std140) uniform CubemapViewBlock
{
	vec2 view;
	int mode;
	float LOD;
} CubemapViewParam;

vec4 CrossView(vec2 uv)
{
	uv += 1.0;
	uv *= 0.5;
	vec4 col = vec4(0.0,0.0,0.0,1.0);
	vec2 ngs = floor(uv * vec2(4.0,3.0)+vec2(1.0,0.0)) * 3.14159 * 0.5;
	vec2 cs = cos(ngs);
	vec2 sn = sin(ngs);
	uv.y = 1.0-uv.y;
	vec3 nd = vec3(0.0,0.0,0.0);
	
	if (uv.y>=0.333 && uv.y<=0.666)
	{
		uv.x =  -(fract(uv.x*4.0) * 2.0 - 1.0);
		vec3 d = vec3(uv.x, uv.y*6.0-3.0, 1.0);
		nd = vec3(d.x*cs.x - d.z*sn.x, d.y, d.x*sn.x + d.z*cs.x);
	}
	else
		if (uv.x>=0.25 && uv.x<=0.5)
		{
			uv.y = fract(uv.y*3.0) * 2.0 - 1.0;
			vec3 d = vec3(uv.x*8.0-3.0, 1.0, uv.y);
			nd = vec3(d.x, d.y*cs.y - d.z*sn.y, d.y*sn.y + d.z*cs.y);
		}

	return textureLod(CubeSampler0, nd, CubemapViewParam.LOD);
}

vec4 getUV(vec2 I)
{
  vec2 a = vec2(2.0*I.x, I.x+I.y);
  vec2 b = vec2(-2.0*I.x,-I.x+I.y);
  vec2 c = vec2(-I.x-I.y, I.x-I.y);
    
  vec4 uv = vec4(0.0);
  
  if(max(max(max(a.x, a.y),max(b.x, b.y)),max(c.x,c.y)) <= 1.0)
  {
    if(I.x >= 0.0 && I.x + I.y >= 0.0)
      uv = vec4(a,0.0, 1.0);
    else if(I.x <= 0.0 && -I.x + I.y >= 0.0)
      uv = vec4(0.0,b.y,b.x, 1.0);
    else
      uv = vec4(c.y,0.0,c.x, 1.0);
  }
  return uv;
}

vec4 IsometricView(vec2 uv)
{
	vec2 I = vec2(uv.x, -uv.y);
  
   	vec4 nuv;
  	if (I.x>0.0)
   		nuv = getUV(I-vec2(0.5,0.0));  
    else
        nuv = vec4(1.0, 1.0, 1.0, 0.0)-getUV(vec2(I.x, -I.y)+vec2(0.5,0.0));  
	return textureLod(CubeSampler0, nuv.xyz*2.0-1.0, CubemapViewParam.LOD) *abs(nuv.w);
}

vec4 Projection(vec2 uv)
{
	vec2 ng = uv * vec2(PI, PI * 0.5);
	vec2 a = cos(ng);
	vec2 b = sin(ng);
	return textureLod(CubeSampler0, normalize(vec3(a.x*a.y, -b.y, b.x*a.y)), CubemapViewParam.LOD); 
}

vec4 CameraView(vec2 uv)
{
	float an = CubemapViewParam.view.x * PI * 2.0;
	float dn = CubemapViewParam.view.y * PI * 0.5;
	float cdn = cos(dn);

	vec3 ro = vec3( 2.0*sin(an)*cdn, sin(dn)*2.0, 2.0*cos(an)*cdn );
    
    vec3 ww = normalize( ro );
    vec3 uu = normalize( cross(ww,vec3(0.0,1.0,0.0) ) );
    vec3 vv = normalize( cross(uu,ww));

	vec3 rd = normalize( uv.x*uu - uv.y*vv + 1.5*ww );

	return textureLod(CubeSampler0, rd, CubemapViewParam.LOD);
}

vec4 CubemapView()
{
	vec2 uv = vUV * 2.0 - 1.0;
	switch(CubemapViewParam.mode)
	{
	 case 0:
		return Projection(uv);
	case 1:
		return IsometricView(uv);
	case 2:
		return CrossView(uv);
	case 3:
		return CameraView(uv);
	}
}
