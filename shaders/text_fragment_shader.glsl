#version 330 core

in vec2 TexCoords;

out vec4 color;

uniform sampler2D textureSampler;
uniform float mixFactor;

void main()
{    
    vec4 texColor = texture(textureSampler, TexCoords);

    float hue = TexCoords.x;
    float r = abs(hue * 6.0 - 3.0) - 1.0;
    float g = 2.0 - abs(hue * 6.0 - 2.0);
    float b = 2.0 - abs(hue * 6.0 - 4.0);
    vec3 hue3 = clamp(vec3(r,g,b), 0, 1);
    vec3 color3 = hue3 * vec3(TexCoords.y);

    vec4 vertColor = vec4(color3, 1.0);

    color = (texColor * (1-mixFactor)) + (vertColor * (mixFactor));
}  