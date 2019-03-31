vec4 GLTFRead()
{
	vec3 lightdir = normalize(vec3(1.0));
	float dt = max(dot(lightdir, normalize(vWorldNormal)), 0.5);
    return vec4(dt, dt, dt, 1.0);
}
