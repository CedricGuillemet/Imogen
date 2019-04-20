layout (std140) uniform FurGeneratorBlock
{
	int hairCount;
	float lengthFactor;
} FurGeneratorParam;


out vec4 outCompute0; // color in w, length in outCompute3.w
out vec4 outCompute1;
out vec4 outCompute2;
out vec4 outCompute3;
out vec4 outCompute4;
out vec4 outCompute5;
out vec4 outCompute6;
out vec4 outCompute7;
out vec4 outCompute8;
out vec4 outCompute9;
out vec4 outCompute10;
out vec4 outCompute11;
out vec4 outCompute12;
out vec4 outCompute13;
out vec4 outCompute14;

uniform sampler2D Sampler0;
uniform sampler2D Sampler1;

void main()
{
	float n = float(gl_VertexID);
	vec2 uv = vec2(mod(n, 32.0), floor(n/32.0)) / 32.0;
	uv *= 4.;
	
	vec4 color = texture(Sampler0, uv);
	float length = texture(Sampler1, uv).x * max(FurGeneratorParam.lengthFactor, 6.);
	
	float ng = n * 2.9781;
	vec4 gp = vec4(uv.x, 0., uv.y, 0.) * 8.0;
	gp.xyz += vec3(cos(ng*1.117), 0., sin(ng*0.914)) * 0.2;
	outCompute0 = gp;
	vec3 nrm = normalize(vec3(0.,1.,0.) + vec3(cos(ng), 0., sin(ng)) * 0.25 );
	outCompute1 = vec4(nrm, 0.);
	outCompute2 = gp + vec4(nrm, 0.) * 0.01;
	outCompute3 = gp + vec4(nrm, 0.) * 0.02;
	
	outCompute0.a = color.r;
	outCompute1.a = color.g;
	outCompute2.a = color.b;
	outCompute3.a = length;
	
	outCompute4 = vec4(0.);
	outCompute5 = vec4(0.);
	outCompute6 = vec4(0.);
	outCompute7 = vec4(0.);
	outCompute8 = vec4(0.);
	outCompute9 = vec4(0.);
	outCompute10 = vec4(0.);
	outCompute11 = vec4(0.);
	outCompute12 = vec4(0.);
	outCompute13 = vec4(0.);
	outCompute14 = vec4(0.);
}