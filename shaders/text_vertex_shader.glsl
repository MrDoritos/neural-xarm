#version 330 core

layout (location = 0) in vec2 vertex;
layout (location = 1) in vec2 texture;

out vec2 TexCoords;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(vertex.x, -vertex.y, 0.0, 1.0);
    TexCoords = texture;
} 
