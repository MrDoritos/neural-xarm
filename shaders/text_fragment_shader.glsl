#version 330 core
in vec2 TexCoords;
out vec4 color;

//uniform sampler2D text;
uniform sampler2D textureSampler;
uniform vec3 textColor;

void main()
{    
    //color = texture(text, TexCoords);
    color = texture(textureSampler, TexCoords);
}  