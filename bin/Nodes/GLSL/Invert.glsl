vec4 Invert()
{
    vec4 res = vec4(1.0,1.0,1.0,1.0) - texture(Sampler0, vUV);
    res.a = 1.0;
    return res;
}
