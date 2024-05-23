#version 450

struct Light{
    vec4 lightPosition;
    vec4 lightColor;
};

layout(set = 0, binding = 0) uniform FrameUniform{
    vec4 cameraPos;
    mat4 proj;
    mat4 view;
    vec4 globalLightColor;
    uint lightsCount;
    Light lights[];
}frameUniform;

layout(set = 0, binding = 1) uniform ObjectUniform{
    mat4 model;
}objUniform;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 outFragColor;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outFragPos;

void main() {
    gl_Position = frameUniform.proj * frameUniform.view * objUniform.model * vec4(inPosition, 1.0);

    outFragColor = frameUniform.cameraPos.xyz;
    outTexCoord = inTexCoord;
    outFragPos = vec3(objUniform.model * vec4(inPosition, 1.0));

    //See https://learnopengl.com/Lighting/Basic-Lighting "One last thing"
    //XXX: this matrices transformations should be done on the application
    //and passed to the vertex as a separated matrix since this is costly
    outNormal = mat3(transpose(inverse(objUniform.model))) * inNormal;
}
