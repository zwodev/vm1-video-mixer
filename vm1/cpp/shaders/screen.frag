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

uniform sampler2D inputTexture0;
uniform sampler2D inputTexture1;
uniform float mixValue;

void main() {
	// Mix images
	vec2 coord = vec2(texCoord.x, 1.0f - texCoord.y);
	vec4 col0 = texture(inputTexture0, coord);
	vec4 col1 = texture(inputTexture1, coord);
	fragColor = mix(col0, col1, mixValue);
}
