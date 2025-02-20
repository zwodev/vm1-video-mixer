/*
 * Copyright (c) 2025 Nils Zweiling
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

uniform sampler2D rgbTexture;

void main() {
	vec2 coord = vec2(texCoord.x, texCoord.y);	

	vec3 col = texture(rgbTexture, coord).rgb;
	fragColor = vec4(col, 1.0f);
}
