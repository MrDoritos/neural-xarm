#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor;

out vec3 worldPos;
out vec3 normal;
out vec2 texCoord;
out vec3 color;

uniform mat4 model, view, projection;
uniform mat3 norm;

void main() {
    vec4 pos_4 = model * vec4(aPosition.xyz, 1.0);
    worldPos = pos_4.xyz;
    gl_Position = projection * view * pos_4;
    normal = normalize(norm * aNormal);
    texCoord = vec2(aTexCoord.x, -aTexCoord.y);
    color = aColor;
}
