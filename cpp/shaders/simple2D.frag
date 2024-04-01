#version 300 es

#ifdef GL_ES
precision highp float;
#endif

in vec4 color;
in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D texSampler;

void main() {
	mediump vec3 yuv;
	lowp vec3 rgb;

	// Get the YUV values
	// Raspberry Pi4 Hardware Deocoder uses SAND128
	vec2 inTexSize = vec2(2048.0f, 2048.0f);
	vec2 outTexSize = vec2(1920.0f, 1080.0f);
	vec2 d = vec2(0.5f/inTexSize.x, 0.5f/inTexSize.y);
	float x = floor(texCoord.x * (outTexSize.x-1.0f) + 0.5f);
	float y = floor(texCoord.y * (outTexSize.y-1.0f) + 0.5f);
	float col = floor(x / 128.0f);
	float posInCol = floor(y * 128.0f + floor(mod(x, 128.0f) + 0.5f) + 0.5f);
	float xNew = floor(mod(posInCol, inTexSize.x) + 0.5f);
	float yNew = col * 102.0f + floor(posInCol / inTexSize.x);
	vec2 uv = vec2((xNew / inTexSize.x) + d.x, (yNew / inTexSize.y) + d.y);
	yuv.x = texture(texSampler, uv).r;

	fragColor = vec4(yuv.x, yuv.x, yuv.x, 1);
}
