#ifdef VERTEX_SHADER

layout(location = 0)in vec2 inUV;
out vec2 vUV;

void main()
{ 
	gl_Position = vec4(inUV.xy*2.0 - 1.0,0.5,1.0); vUV = inUV; 
}

#endif

#ifdef FRAGMENT_SHADER

uniform float time;
#define PI 3.1415926
layout(location = 0) out vec4 outPixDiffuse;
in vec2 vUV;

void main() 
{
	vec2 npos = vUV-0.5;
	float mixcontrol = sin(time);
	if (mixcontrol < 0.0) 
	{ 
		mixcontrol = pow(1.0 - abs(mixcontrol), 3.0) - 1.0; 
	} 
	else 
	{ 
	mixcontrol = 1.0 - pow(1.0 - mixcontrol, 3.0); 
	}

	mixcontrol = mixcontrol * 0.5 + 0.5;
	float c1time = time * 2.0;
	vec2 c1pos = npos + vec2(sin(c1time), cos(c1time)) * 0.24;
	float c1size = 0.05;
	float c2time = time * 2.0 + PI;
	vec2 c2pos = npos + vec2(sin(c2time), cos(c2time)) * 0.24;
	float c2size = 0.05;
	c1pos = mix(npos, c1pos, mixcontrol);
	c1size = mix(0.18, c1size, mixcontrol);
	c2pos = mix(npos, c2pos, 1.0 - mixcontrol);
	c2size = mix(0.18, c2size, 1.0 - mixcontrol);
	vec3 colorbg = vec3(32.0 / 255.0);
	vec3 colorfg = vec3(250.0 / 255.0);
	vec4 col = vec4(colorbg, 1.0);
	if (length(npos) < 0.3) 
	{ 
		col = vec4(colorfg, 1.0); 
	}
	if (length(c1pos) < c1size) 
	{ 
		col = vec4(colorbg, 1.0); 
	}
	if (length(c2pos) < c2size) 
	{ 
		col = vec4(colorbg, 1.0); 
	}
	outPixDiffuse = vec4(col.rgb, 1.0); 
}

#endif
