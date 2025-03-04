#version 330 core

in vec3 worldPos;
in vec3 normal;
in vec2 texCoord;

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
    //// FragColor = vec4(0, 1.0, 0, 1.0);
    //FragColor.rg = vec2(0.0);
    //float dotN = dot(normal, vec3(0,1,0)) + 1.0;
    //FragColor.b = (0.3 * (pow(dotN, 0.9))) + 0.3;
    //FragColor.a = 1.0;
    //return;

    mat4 model = mat4(-norm);
    mat3 mv_norm = mat3(transpose(inverse(model)));

    vec4 l_pos = vec4(20,30,0,1);
    vec3 l_norm = normalize(l_pos.xyz);
    vec3 v_normal = normalize(normal);
    vec3 w_normal = normalize(normal * mv_norm);
    vec4 w_4 = vec4(worldPos.xyz, 1.0);

    vec3 normalMix = normalize(normal) * norm;
    vec3 wv_n = normalize(eyePos - worldPos);
    vec3 view = normalize(wv_n * mv_norm);

    float approach = abs(dot(view, normalMix));
    vec3 v_to_l = vec3(model * w_4 - l_pos);
    vec3 l_dir = normalize(l_pos.xyz - worldPos);
    //float dotSky = clamp(dot(w_normal, l_dir)*0.1,0.0,1.0);
    //float dotSky = dot(l_norm, wv_n) * 5;
    //float dotwl = dot(l_norm, normalize(worldPos));
    //float dotwl = dot(l_norm, v_normal);
    //float dotSky = pow(clamp(dotwl, 0.0, 1.0)*5,5);
    //float dotSky = dot(view, w_normal);
    //float dotSky = max(dot(normalize(normal * mv_norm), normalize(l_pos.xyz - worldPos)), 0.0);
    float dotSky = max(dot(l_dir, normalize(normal * mv_norm)), 0.0);

    vec3 diffuseColor = texture(material.diffuse, texCoord).rgb;
    vec3 specularColor = texture(material.specular, texCoord).rgb;    
    
    vec3 sDiffuse = light.diffuse * diffuseColor * dotSky;
    vec3 sAmbient = light.ambient * diffuseColor;// * vec3(material.shininess);
    vec3 sSpecular = light.specular * specularColor;

    vec3 sMix = sDiffuse + sAmbient + sSpecular;

    FragColor.rgb = sMix;
    FragColor.a = 1.0;

    return;

    
    vec3 lightDir = normalize(light.position - worldPos);
    vec3 halfway = normalize(view + lightDir);
    float lightDist = length(light.position - worldPos);
    float dotNL = max(dot(normal, lightDir), 0.0);
    float dotNH = max(dot(normal, halfway), 0.0);


    // attenuation value to cover a distance of 32
    // see https://learnopengl.com/Lighting/Light-casters
    //float atten = 1.0 / (1.0 + 0.14 * lightDist + 0.07 * lightDist * lightDist);
    float atten = 1.0;
    vec3 Lambient = light.ambient * diffuseColor;
    vec3 Ldiffuse = light.diffuse * diffuseColor * dotNL * atten;
    vec3 Lspecular = light.specular * specularColor * pow(dotNH, material.shininess) * atten;
    vec3 Lo = Lambient + Ldiffuse + Lspecular;

    vec3 LoMapped = Lo / (Lo + vec3(1.0));
    FragColor.rgb = pow(LoMapped, vec3(1.0 / gamma));
    FragColor.a = 1.0;
}