#version 450        // Use GLSL 4.5

layout(location = 0) in vec3 fragColour;    // Interpolated colour from vertex shader (layout location must match vertex shader)
layout(location = 1) in vec2 fragTexture;   // Texture fragment from vertex shader

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColour;    // Final output colour (must also have layout location, which is separate from 'in' variables)

void main() {
    outColour = texture(textureSampler, fragTexture);
}
