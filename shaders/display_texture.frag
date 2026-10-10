#version 410

in vec2 texCoord;
out vec4 fragColour;

uniform sampler2D frameTexture;

void main() {
    // Sample the rendered scene texture
    fragColour = texture(frameTexture, texCoord);
}
