#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 fragPos;

struct Light{
    vec4 lightPosition;
    vec4 lightColor;
};

layout(set = 0, binding = 0) uniform FrameUniform{
    vec4 cameraPos;
    mat4 proj;
    mat4 view;
    vec4 globalLightColor;
    Light lights[];
    uint lightsCount;
}frameUniform;

layout(set = 0, binding = 2) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    float ambientStrength = 0.1;
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 ambient = ambientStrength * lightColor;

    vec3 normalizedNormal = normalize(normal);
    vec3 lightDir = normalize(frameUniform.lights[1].lightPosition.xyz - fragPos);
    float diff = max(dot(normalizedNormal, lightDir), 0.0);
    vec3 diffuse = diff * frameUniform.lights[1].lightColor.xyz; 

    float specularStrength = 0.9;
    vec3 cameraDir = normalize(frameUniform.cameraPos.xyz - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(cameraDir, reflectDir), 0.0), 256);
    vec3 specular = specularStrength * spec * frameUniform.lights[1].lightColor.xyz;
    
    vec3 objColor = texture(texSampler, texCoord).xyz;
    outColor = vec4((ambient + diffuse + specular) * objColor, 1.0);
    //outColor = vec4(frameUniform.lights[1].lightPosition.xyz, 1.0);
}
