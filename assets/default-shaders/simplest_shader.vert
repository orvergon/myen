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

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 texPosition;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 texCoord;

void main() {
    gl_Position = frameUniform.proj * frameUniform.view * objUniform.model * vec4(inPosition.xy, 0.0, 1.0);

    fragColor = frameUniform.cameraPos.xyz;
    texCoord = texPosition;
}
