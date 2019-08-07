$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

uniform vec4 u_time;


void main()
{
	vec2 npos = v_texcoord0-vec2(0.5, 0.5);
	float mixcontrol = sin(u_time.x);
	if (mixcontrol < 0.0) 
	{ 
		mixcontrol = pow(1.0 - abs(mixcontrol), 3.0) - 1.0; 
	} 
	else 
	{ 
		mixcontrol = 1.0 - pow(1.0 - mixcontrol, 3.0); 
	}

	mixcontrol = mixcontrol * 0.5 + 0.5;
	float c1time = u_time.x * 2.0;
	vec2 c1pos = npos + vec2(sin(c1time), cos(c1time)) * 0.24;
	float c1size = 0.05;
	float c2time = u_time.x * 2.0 + PI;
	vec2 c2pos = npos + vec2(sin(c2time), cos(c2time)) * 0.24;
	float c2size = 0.05;
	c1pos = mix(npos, c1pos, mixcontrol);
	c1size = mix(0.25, c1size, mixcontrol);
	c2pos = mix(npos, c2pos, 1.0 - mixcontrol);
	c2size = mix(0.25, c2size, 1.0 - mixcontrol);
	float c1 = 60.0 / 255.0;
	float c2 = 250.0 / 255.0;
	vec3 colorbg = vec3(c1, c1, c1);
	vec3 colorfg = vec3(c2, c2, c2);
	vec4 col = vec4(colorbg, 1.0);
	
	float th = 0.01;
	col = mix(vec4(colorfg, 1.0), col, smoothstep(0.3, 0.3+th, length(npos)));
	col = mix(vec4(colorbg, 1.0), col, smoothstep(c1size, c1size+th, length(c1pos)));
	col = mix(vec4(colorbg, 1.0), col, smoothstep(c2size, c2size+th, length(c2pos)));
	
	gl_FragColor = vec4(col.rgb, 1.0); 
}
