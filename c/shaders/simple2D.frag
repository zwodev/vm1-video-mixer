#version 300 es

#ifdef GL_ES
precision highp float;
#endif

in vec4 color;
in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D texSampler;

void main() {
	vec4 color = texture(texSampler, texCoord);
	fragColor = color;
	//fragColor = vec4(texCoord.x, texCoord.y, 0, 1);
}
