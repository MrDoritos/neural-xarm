#version 330 core

in vec3 worldPos;
in vec3 normal;
in vec2 texCoord;
in vec3 color;

out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

const float gamma = 1.0;

uniform vec3 eyePos;
uniform Material material;
uniform Light light;
uniform mat3 norm;

void main() {
    vec4 l_pos = vec4(-20,20,0,1);

    vec3 l_dir = normalize(l_pos.xyz - worldPos);
    vec3 v_dir = normalize(eyePos - worldPos);
    vec3 w_dir = normalize(worldPos);
    
    float dotSky = max(dot(l_dir, normalize(normal)), 0.0);
    float dotView = pow(max(1-abs(dot(v_dir, l_dir)), 0.0), material.shininess);

    vec3 diffuseColor = texture(material.diffuse, texCoord).rgb + color;
    vec3 specularColor = texture(material.specular, texCoord).rgb + color;    
    
    vec3 sDiffuse = light.diffuse * diffuseColor * dotSky;
    vec3 sAmbient = light.ambient * diffuseColor;
    vec3 sSpecular = light.specular * specularColor * dotView;

    vec3 sMix = sDiffuse + sAmbient + sSpecular;

    FragColor.rgb = sMix;
    FragColor.a = 1.0;
}