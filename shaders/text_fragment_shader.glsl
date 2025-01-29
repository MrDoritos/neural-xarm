#version 330 core

in vec4 TexCoords;

out vec4 color;

uniform sampler2D textureSampler;
uniform float mixFactor;

void main()
{    
    vec4 texColor = texture(textureSampler, TexCoords.xy);
    vec4 vertColor = TexCoords;
    float mix_amt = clamp(mixFactor,0,1);

    if (mixFactor < -.1) {
        color = texColor + vertColor;
        color.a = abs(mixFactor+0.5)+vertColor.a;
        return;
    }

    color = (texColor * (1-mix_amt)) + (vertColor * (mix_amt));
}  