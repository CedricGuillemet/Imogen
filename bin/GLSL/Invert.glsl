vec4 Invert()
{
    return vec4(1.0) - texture(Sampler0, vUV);
}
