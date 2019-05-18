
layout (std140) uniform NGonBlock
{
	int sides;
	float radius;
	float t;
    float angle;
};

vec4 NGon()
{
    vec2 p = vUV - vec2(0.5);
    
    vec2 d = vec2(cos(angle), sin(angle));
    float ng = 0.0;
    float col = 0.0;
    
    for(int i = 0;i<sides;i++)
    {
        vec2 rd = Rotate2D(d, ng);
     	ng += 3.14159*2.0 / float(sides);
        float d = smoothstep(mix(-radius, 0.0, t), 0.001, dot(p,rd)-radius);
        col = max(col, d);
    }
	col = 1.0 - col;
	return vec4(col);
}
