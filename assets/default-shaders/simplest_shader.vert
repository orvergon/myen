#version 450

struct FrameUniformStruct{
    vec4 cameraPos;
//    mat4 proj;
//    mat4 view;
};

layout(set = 0, binding = 0) uniform FrameUniform{
    vec4 cameraPos;
    mat4 proj;
    mat4 view;
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

void main() {
    gl_Position = frameUniform.proj * frameUniform.view * objUniform.model * vec4(inPosition, 1.0);

    outFragColor = frameUniform.cameraPos.xyz;
    outTexCoord = inTexCoord;
    outNormal = inNormal;
}
