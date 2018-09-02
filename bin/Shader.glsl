#define PI 3.14159265359

#ifdef VERTEX_SHADER

layout(location = 0)in vec2 inUV;
out vec2 vUV;

void main()
{
    gl_Position = vec4(inUV.xy*2.0-1.0,0.5,1.0); 
	vUV = inUV;
}

#endif


#ifdef FRAGMENT_SHADER

layout(location=0) out vec4 outPixDiffuse;
in vec2 vUV;

uniform sampler2D Sampler0;
uniform sampler2D Sampler1;
uniform sampler2D Sampler2;
uniform sampler2D Sampler3;


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

//===============================================================================================
// from Iq : https://www.shadertoy.com/view/MtsGWH
vec4 boxmap( sampler2D sam, in vec3 p, in vec3 n, in float k )
{
    vec3 m = pow( abs(n), vec3(k) );
	vec4 x = texture( sam, p.yz );
	vec4 y = texture( sam, p.zx );
	vec4 z = texture( sam, p.xy );
	return (x*m.x + y*m.y + z*m.z)/(m.x+m.y+m.z);
}

vec4 LambertMaterial(vec2 uv, vec2 view)
{
	vec2 p = uv *vec2(2.0,-2.0) +vec2(- 1.0, 1.0);

     // camera movement	
	float an = view.x * PI * 2.0;
	float dn = view.y * PI * 0.5;
	float cdn = cos(dn);
	vec3 ro = vec3( 2.5*sin(an)*cdn, 1.0 + sin(dn)*2.0, 2.5*cos(an)*cdn );
    vec3 ta = vec3( 0.0, 1.0, 0.0 );
    // camera matrix
    vec3 ww = normalize( ta - ro );
    vec3 uu = normalize( cross(ww,vec3(0.0,1.0,0.0) ) );
    vec3 vv = normalize( cross(uu,ww));
	// create view ray
	vec3 rd = normalize( p.x*uu + p.y*vv + 1.5*ww );

    // sphere center	
	vec3 sc = vec3(0.0,1.0,0.0);

    vec3 col = vec3(0.0);
    
	// raytrace-plane
	float h = (0.0-ro.y)/rd.y;
	if( h>0.0 ) 
	{ 
		vec3 pos = ro + h*rd;
		vec3 nor = vec3(0.0,1.0,0.0); 
		vec3 di = sc - pos;
		float l = length(di);
		float occ = 1.0 - dot(nor,di/l)*1.0*1.0/(l*l); 

        col = texture( Sampler0, 0.25*pos.xz ).xyz;
        col *= occ;
        col *= exp(-0.1*h);
	}

	// raytrace-sphere
	vec3  ce = ro - sc;
	float b = dot( rd, ce );
	float c = dot( ce, ce ) - 1.0;
	h = b*b - c;
	if( h>0.0 )
	{
		h = -b - sqrt(h);
        vec3 pos = ro + h*rd;
        vec3 nor = normalize(ro+h*rd-sc); 
        float occ = 0.5 + 0.5*nor.y;
        
        col = boxmap( Sampler0, pos, nor, 32.0 ).xyz;
        col *= occ;
    }

	col = sqrt( col );
	
	return vec4( col, 1.0 );
}

vec4 MADD(vec2 uv, vec4 color0, vec4 color1)
{
    return texture(Sampler0, uv) * color0 + color1;
}

float Hexagon(vec2 uv)
{
	vec2 V = vec2(.866,.5);
    vec2 v = abs ( ((uv * 2.0)-1.0) * mat2( V, -V.y, V.x) );	

	return ceil( 1. - max(v.y, dot( v, V)) *1.15  );
}

vec4 Blend(vec2 uv, vec4 A, vec4 B)
{
    return texture(Sampler0, uv) * A + texture(Sampler1, uv) * B;
}

vec4 Invert(vec2 uv)
{
    return vec4(1.0) - texture(Sampler0, uv);
}


void main() 
{ 
	outPixDiffuse = vec4(__FUNCTION__);
	outPixDiffuse.a = 1.0;
}

#endif
