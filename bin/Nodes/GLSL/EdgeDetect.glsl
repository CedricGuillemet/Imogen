layout (std140) uniform EdgeDetectBlock
{
	float radius;
} EdgeDetectParam;

// Use these parameters to fiddle with settings
float step = 1.0;

float intensity(in vec4 color){
	return sqrt((color.x*color.x)+(color.y*color.y)+(color.z*color.z));
}

vec4 ctap(vec2 p)
{
	return texture(Sampler0, p);
}

vec3 sobel(float stepx, float stepy, vec2 center){
	// get samples around pixel
    float tleft = intensity(ctap(center + vec2(-stepx,stepy)));
    float left = intensity(ctap(center + vec2(-stepx,0)));
    float bleft = intensity(ctap(center + vec2(-stepx,-stepy)));
    float top = intensity(ctap(center + vec2(0,stepy)));
    float bottom = intensity(ctap(center + vec2(0,-stepy)));
    float tright = intensity(ctap(center + vec2(stepx,stepy)));
    float right = intensity(ctap(center + vec2(stepx,0)));
    float bright = intensity(ctap(center + vec2(stepx,-stepy)));
 
	// Sobel masks (see http://en.wikipedia.org/wiki/Sobel_operator)
	//        1 0 -1     -1 -2 -1
	//    X = 2 0 -2  Y = 0  0  0
	//        1 0 -1      1  2  1
	
	// You could also use Scharr operator:
	//        3 0 -3        3 10   3
	//    X = 10 0 -10  Y = 0  0   0
	//        3 0 -3        -3 -10 -3
 
    float x = tleft + 2.0*left + bleft - tright - 2.0*right - bright;
    float y = -tleft - 2.0*top - tright + bleft + 2.0 * bottom + bright;
    float color = 1.0 - sqrt((x*x) + (y*y));
    return vec3(color,color,color);
 }


vec4 EdgeDetect()
{
	float e = EdgeDetectParam.radius;
	return vec4(sobel(e, e, vUV), 1.0);
}
