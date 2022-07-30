#version 450        // Use GLSL 4.5

// Vertex information
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
layout(location = 2) in vec2 tex;

layout(set = 0, binding = 0) uniform UboViewProjection {
    mat4 projection;
    mat4 view;
} uboViewProjection;

layout(push_constant) uniform PushModel {
    mat4 model;
} pushModel;

layout(location = 0) out vec3 fragColour;   // Output colour for vertex (layout location is required for Vulkan SPIR-V)
layout(location = 1) out vec2 fragTexture;  // Output coordinate for texture

void main() {
    gl_Position = uboViewProjection.projection * uboViewProjection.view * pushModel.model * vec4(pos, 1.0);

    fragColour = col;
    fragTexture = tex;
}
