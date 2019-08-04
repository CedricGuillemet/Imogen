$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

vec4 u_ambient;
vec4 u_lightdir, u_Kr;
float u_rayleigh_brightness, u_mie_brightness, u_spot_brightness, u_scatter_strength, u_rayleigh_strength, u_mie_strength;
float u_rayleigh_collection_power, u_mie_collection_power, u_mie_distribution;
int u_size;


float surface_height = 0.99;
float range = 0.01;
float intensity = 1.8;
const int step_count = 16;

vec3 get_world_normal(vec2 texcoord0)
{
	vec3 dir = mul(u_viewRot, vec4(texcoord0 * 2.0 - 1.0, 1.0, 0.0)).xyz;
	return normalize(dir);
}

float atmospheric_depth(vec3 position, vec3 dir)
{
	float a = dot(dir, dir);
	float b = 2.0*dot(dir, position);
	float c = dot(position, position)-1.0;
	float det = b*b-4.0*a*c;
	float detSqrt = sqrt(det);
	float q = (-b - detSqrt)/2.0;
	float t1 = c/q;
	return t1;
}

float phase(float alpha, float g)
{
	float a = 3.0*(1.0-g*g);
	float b = 2.0*(2.0+g*g);
	float c = 1.0+alpha*alpha;
	float d = pow(1.0+g*g-2.0*g*alpha, 1.5);
	return (a/b)*(c/d);
}

float horizon_extinction(vec3 position, vec3 dir, float radius)
{
	float u = dot(dir, -position);
	if(u<0.0){
		return 1.0;
	}
	vec3 near = position + u*dir;
	if(length(near) < radius){
		return 0.0;
	}
	else{
		vec3 v2 = normalize(near)*radius - position;
		float diff = acos(dot(normalize(v2), dir));
		return smoothstep(0.0, 1.0, pow(diff*2.0, 3.0));
	}
}

vec3 absorb(float dist, vec3 color, float factor)
{
	float d = factor/dist;
	return color-color*pow(u_Kr.xyz, vec3(d, d, d));
}

void main()
{
	vec3 eyedir = get_world_normal(v_texcoord0);
	vec3 ld = normalize(u_lightdir.xyz);
	float alpha = dot(eyedir, ld);
	float rayleigh_factor = phase(alpha, -0.01)*u_rayleigh_brightness;
	float mie_factor = phase(alpha, u_mie_distribution)*u_mie_brightness;
	float spot = smoothstep(0.0, 15.0, phase(alpha, 0.9995))*u_spot_brightness;
	vec3 eye_position = vec3(0.0, surface_height, 0.0);
	float eye_depth = atmospheric_depth(eye_position, eyedir);
	float step_length = eye_depth/float(step_count);
	float eye_extinction = horizon_extinction(eye_position, eyedir, surface_height-0.15);
	vec3 rayleigh_collected = vec3(0.0, 0.0, 0.0);
	vec3 mie_collected = vec3(0.0, 0.0, 0.0);
	for(int i=0; i<step_count; i++)
	{
		float sample_distance = step_length*float(i);
		vec3 position = eye_position + eyedir*sample_distance;
		float extinction = horizon_extinction(position, ld.xyz, surface_height-0.35);
		float sample_depth = atmospheric_depth(position, ld.xyz);
		vec3 influx = absorb(sample_depth, vec3(intensity, intensity, intensity), u_scatter_strength)*extinction;
		rayleigh_collected += absorb(sample_distance, u_Kr.xyz*influx, u_rayleigh_strength);
		mie_collected += absorb(sample_distance, influx, u_mie_strength);
	}
	
    rayleigh_collected = (rayleigh_collected*eye_extinction*pow(eye_depth, u_rayleigh_collection_power))/float(step_count);
	mie_collected = (mie_collected*eye_extinction*pow(eye_depth, u_mie_collection_power))/float(step_count);
	vec3 color = vec3(spot*mie_collected + mie_factor*mie_collected + rayleigh_factor*rayleigh_collected);
	gl_FragColor = vec4(max(color * u_ambient.w, u_ambient.xyz), 1.0);
}

