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
	vec4 col0 = texture(yuvTexture0, texCoord);
	vec4 col1 = texture(yuvTexture1, texCoord);
	fragColor = mix(vec4(col0.r, col0.r, col0.r, 1.0f), vec4(col1.r, col1.r, col1.r, 1.0f), mixValue);
}
