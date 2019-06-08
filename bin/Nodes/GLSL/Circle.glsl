layout (std140) uniform CircleBlock 
{ 
	float radius; 
	float t; 
};

vec4 Circle()
{
    return vec4(Circle(vUV, radius, t));
}
