/*
 * Copyright (c) 2023-2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#version 310 es

precision mediump float;

in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D inputTexture;

const vec2 OUT_TEX_SIZE = vec2(1920.0f, 1080.0f);

void main() {
	vec2 coord = vec2(texCoord.x, texCoord.y);	

	// YUYV (for camera) 
	float pixelX = floor(coord.x * OUT_TEX_SIZE.x);

    // Sample YUYV texel (contains Y0,U,Y1,V)
    vec4 yuyv = texture(inputTexture, coord);

	//float y = (mod(pixelX, 2.0) < 1.0) ? yuyv.g : yuyv.a;
	float y = mix(yuyv.g, yuyv.a, floor(mod(pixelX, 2.0)));
    float u = yuyv.b - 0.5f;
    float v = yuyv.r - 0.5f;

	float r = y + (1.403f * v);
	float g = y - (0.344f * u) - (0.714f * v);
	float b = y + (1.770f * u);

	vec4 col = vec4(r, g, b, 1.0f);

	// Mix images
	fragColor = col;
}
