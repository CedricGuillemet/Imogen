
vec2 Rotate2D(vec2 v, float a) 
{
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

float Circle(vec2 uv, float radius, float t)
{
    float r = length(uv-vec2(0.5));
    float h = sin(acos(r/radius));
    return mix(1.0-smoothstep(radius-0.001, radius, length(uv-vec2(0.5))), h, t);
}

float Square(vec2 uv, float width)
{
	uv -= vec2(0.5);
    return 1.0-smoothstep(width-0.001, width, max(abs(uv.x), abs(uv.y)));
}

float Checker(vec2 uv)
{
	uv -= vec2(0.5);
    return mod(floor(uv.x)+floor(uv.y),2.0);
}

vec4 Transform(vec2 uv, vec2 translate, float rotate, float scale)
{
	vec2 rs = (uv+translate) * scale;   
    vec2 ro = vec2(rs.x*cos(rotate) - rs.y * sin(rotate), rs.x*sin(rotate) + rs.y * cos(rotate));
    vec2 nuv = fract(ro);
	vec4 tex = texture(Sampler0, nuv);
	return tex;
}

float Sine(vec2 uv, float freq, float angle)
{
	uv -= vec2(0.5);
	uv = vec2(cos(angle), sin(angle)) * (uv * freq * PI * 2.0);
    return cos(uv.x + uv.y) * 0.5 + 0.5;
}

vec4 SmoothStep(vec2 uv, float low, float high)
{
	vec4 tex = texture(Sampler0, uv);
	return smoothstep(low, high, tex);
}

vec4 Pixelize(vec2 uv, float scale)
{
	vec4 tex = texture(Sampler0, floor(uv*scale)/scale);
	return tex;
}

vec4 Blur(vec2 uv, float angle, float strength)
{
	vec2 dir = vec2(cos(angle), sin(angle));
	vec4 col = vec4(0.0);
	for(float i = -5.0;i<=5.0;i += 1.0)
	{
		col += texture(Sampler0, uv + dir * strength * i);
	}
	col /= 11.0;
	return col;
}

vec4 NormalMap(vec2 uv, float spread)
{
	vec3 off = vec3(-1.0,0.0,1.0) * spread;

    float s11 = texture(Sampler0, uv).x;
    float s01 = texture(Sampler0, uv + off.xy).x;
    float s21 = texture(Sampler0, uv + off.zy).x;
    float s10 = texture(Sampler0, uv + off.yx).x;
    float s12 = texture(Sampler0, uv + off.yz).x;
    vec3 va = normalize(vec3(spread,0.0,s21-s01));
    vec3 vb = normalize(vec3(0.0,spread,s12-s10));
    vec4 bump = vec4( normalize(cross(va,vb))*0.5+0.5, s11 );
	return bump;
}


vec4 MADD(vec2 uv, vec4 color0, vec4 color1)
{
    return texture(Sampler0, uv) * color0 + color1;
}

vec4 Color(vec2 uv, vec4 color)
{
	return color;
}

float Hexagon(vec2 uv)
{
	vec2 V = vec2(.866,.5);
    vec2 v = abs ( ((uv * 2.0)-1.0) * mat2( V, -V.y, V.x) );	

	return ceil( 1. - max(v.y, dot( v, V)) *1.15  );
}

vec4 Blend(vec2 uv, vec4 A, vec4 B, int op)
{
	switch (op)
	{
	case 0: // add
		return texture(Sampler0, uv) * A + texture(Sampler1, uv) * B;
	case 1: // mul
		return texture(Sampler0, uv) * A * texture(Sampler1, uv) * B;
	case 2: // min
		return min(texture(Sampler0, uv) * A, texture(Sampler1, uv) * B);
	case 3: // max
		return max(texture(Sampler0, uv) * A, texture(Sampler1, uv) * B);
	}
}

vec4 Invert(vec2 uv)
{
    return vec4(1.0) - texture(Sampler0, uv);
}

vec4 CircleSplatter(vec2 uv, vec2 distToCenter, vec2 radii, vec2 angles, float count)
{
	vec4 col = vec4(0.0);
	for (float i = 0.0 ; i < count ; i += 1.0)
	{
		float t = i/(count-0.0);
		vec2 dist = vec2(mix(distToCenter.x, distToCenter.y, t), 0.0);
		dist = Rotate2D(dist, mix(angles.x, angles.y, t));
		float radius = mix(radii.x, radii.y, t);
		col = max(col, vec4(Circle(uv-dist, radius, 0.0)));
	}
	return col;
}

float GetRamp(float v, vec2 arr[8]) 
{
    for (int i = 0;i<(arr.length()-1);i++)
    {
        if (v >= arr[i].x && v <= arr[i+1].x)
        {
            // linear
            //float t = (v-arr[i].x)/(arr[i+1].x-arr[i].x);
            // smooth
            float t = smoothstep(arr[i].x, arr[i+1].x, v);
            return mix(arr[i].y, arr[i+1].y, t);
        }
    }
    
    return 0.0;
}

vec4 Ramp(vec2 uv, vec2 ramp[8])
{
	vec4 tex = texture(Sampler0, uv);
	return tex * GetRamp(tex.x, ramp);
}

vec4 GetTile(vec2 uv)
{
    vec2 t = floor((floor(uv)+1.0)*0.5);
    uv += t *0.1;
    vec2 md = mod(uv, 2.0);
    
    if (md.x > 1.0 || md.y > 1.0)
        return vec4(0.0);
    
    return texture(Sampler0, uv);
}

vec4 Tile(vec2 uv, float scale, vec2 offset0, vec2 offset1, vec2 overlap)
{
	vec2 nuv = uv*scale;

    vec4 col = GetTile(nuv+offset0);
    col += GetTile(nuv + vec2(0.95, 0.0)+offset0);
    col += GetTile(nuv + vec2(0.0, 0.95)+offset1);    
    col += GetTile(nuv + vec2(0.95, 0.95)+offset1); 
	
	return col;
}
