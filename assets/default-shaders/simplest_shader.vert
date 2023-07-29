#version 450

struct FrameUniformStruct{
    vec4 cameraPos;
//    mat4 proj;
//    mat4 view;
};

layout(set = 0, binding = 0) uniform FrameUniform{
    vec4 cameraPos;
}FU;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 texPosition;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 texCoord;

void main() {
    gl_Position = vec4(inPosition.xy, 0.0, 1.0);
    fragColor = FU.cameraPos.xyz;
    texCoord = texPosition;
}
