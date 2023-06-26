#version 450

struct ColorUniform{
    vec3 color;
};

layout(set = 0, binding = 0) uniform ColorU{
    ColorUniform color;
}CU;

layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition.xy, 0.0, 1.0);
    fragColor = CU.color.color;
}
