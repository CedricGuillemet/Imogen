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


float Circle(vec2 uv, float radius)
{
    return 1.0-smoothstep(radius-0.001, radius, length(uv-vec2(0.5)));
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
	//uv -= vec2(0.5);
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



void main() 
{ 
	outPixDiffuse = vec4(__FUNCTION__);
	outPixDiffuse.a = 1.0;
}

#endif
