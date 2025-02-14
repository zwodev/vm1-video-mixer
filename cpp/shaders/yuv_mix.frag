/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#version 310 es

precision mediump float;

in vec4 color;
in vec2 texCoord;
in float offset;

out vec4 fragColor;

uniform sampler2D yuvTexture0;
uniform sampler2D yuvTexture1;
uniform float mixValue;

void main() {
	// Mix images
	vec2 coord = vec2(texCoord.x, 1.0f - texCoord.y);
	float col0 = texture(yuvTexture0, coord).r;
	float col1 = texture(yuvTexture1, coord).r;
	float mixed = mix(col0, col1, mixValue);
	fragColor = vec4(mixed, mixed, mixed, 1.0f);
}
