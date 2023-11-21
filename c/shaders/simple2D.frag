#version 300 es

#ifdef GL_ES
precision highp float;
#endif

in vec4 color;
in vec2 texCoord;

out vec4 fragColour;

void main() {
	fragColour = vec4(texCoord.x, texCoord.y, 0, 1);
}
