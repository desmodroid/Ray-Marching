#version 410

layout (location = 0) in vec3 position;
layout (location = 3) in vec2 texCoordIn;

out vec2 texCoord;

void main() {
    // Assume input position is already in clip coordinates
    gl_Position = vec4(position, 1.0);
    texCoord = texCoordIn;
}