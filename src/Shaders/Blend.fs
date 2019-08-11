$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

uniform vec4 u_A;
uniform vec4 u_B;
uniform vec4 u_op;

void main()
{
	vec4 res;
    vec4 a = texture2D(Sampler0, v_texcoord0) * u_A;
    vec4 b = texture2D(Sampler1, v_texcoord0) * u_B;
    vec4 white = vec4(1.0, 1.0, 1.0, 1.0);
	int op = int(u_op.x);
	if (op == 0) // Add
		res = a + b;
	else if (op == 1) // Multiply
		res = a * b;
	else if (op == 2) // Darken
		res = min(a, b);
	else if (op == 3) // Lighten
		res = max(a, b);
	else if (op == 4) // Average
		res = (a + b) * 0.5;
	else if (op == 5) // Screen
		res = white - ((white - b) * (white - a));
	else if (op == 6) // Color Burn
		res = white - (white - a) / b;
	else if (op == 7) // Color Dodge
		res = a / (white - b);
	else if (op == 8) // Soft Light
		res = 2.0 * a * b + a * a - 2.0 * a * a * b;
	else if (op == 9) // Subtract
		res = a - b;
	else if (op == 10) // Difference
		res = abs(b - a);
	else if (op == 11) // Inverse Difference
		res = white - abs(white - a - b);
	else if (op == 12) // Exclusion
		res = b + a - (2.0 * a * b);
		
	gl_FragColor = res;
}