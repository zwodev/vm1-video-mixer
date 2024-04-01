#version 300 es

layout(location = 0) in vec2 in_Position;
layout(location = 1) in vec2 in_TexCoord;

out vec4 color;
out vec2 texCoord;

const lowp vec4 white = vec4(1.0);

void main() {
	color = white;
	texCoord = in_TexCoord; 
	gl_Position = vec4(in_Position, 0.0, 1.0);
}
