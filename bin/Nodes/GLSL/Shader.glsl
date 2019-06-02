
#define PI 3.14159265359
#define SQRT2 1.414213562373095

#define TwoPI (PI*2)

#ifdef VERTEX_SHADER

attribute vec2 inUV;
attribute vec4 inColor;
attribute vec3 inPosition;
attribute vec3 inNormal;
varying vec2 vUV;
varying vec3 vWorldPosition;
varying vec3 vWorldNormal;
varying vec4 vColor;

void main()
{
	vUV = inUV;
	vColor = inColor;
	gl_Position = vec4(inUV.xy*2.0-1.0,0.5,1.0); 
	vWorldPosition = vec3(0.,0.,0.);
	vWorldNormal = vec3(0.,0.,0.);
}

#endif


#ifdef FRAGMENT_SHADER

varying vec2 vUV;
varying vec3 vWorldPosition;
varying vec3 vWorldNormal;
varying vec4 vColor;


__NODE__



void main() 
{ 
	//outPixDiffuse = vec4(__FUNCTION__);
	gl_FragColor = vec4(__FUNCTION__);
}

#endif
