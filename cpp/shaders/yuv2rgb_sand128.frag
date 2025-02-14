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

uniform sampler2D yuvTexture;

const vec2 IN_TEX_SIZE = vec2(2048.0f, 1530.0f);
const vec2 IN_TEX_SIZE_INV = 1.0f / IN_TEX_SIZE;
const vec2 OUT_TEX_SIZE = vec2(1920.0f, 1080.0f);
const vec2 STEP = 1.0f / IN_TEX_SIZE;
const vec2 D = 0.5f / IN_TEX_SIZE;
const float COLUMN_WIDTH = 128.0f;
const float COLUMN_WIDTH_INV = 1.0f / 128.0f;

vec2 getYCoord(vec2 coord)
{
	float x = floor(coord.x * OUT_TEX_SIZE.x);
	float y = floor(coord.y * OUT_TEX_SIZE.y);
	float col = floor(x / COLUMN_WIDTH);
	float posInCol = y * COLUMN_WIDTH + floor(mod(x, COLUMN_WIDTH));
	float xNew = mod(posInCol, IN_TEX_SIZE.x);
	float yNew = col * 102.0f + floor(posInCol / IN_TEX_SIZE.x);
	vec2 uv = vec2((xNew / IN_TEX_SIZE.x) + D.x, (yNew / IN_TEX_SIZE.y) + D.y);
	return uv;
}

vec2 getUVCoord(vec2 coord)
{
	float x = floor(coord.x * OUT_TEX_SIZE.x);
	float y = floor(coord.y * OUT_TEX_SIZE.y * 0.5f);
	float col = floor(x / COLUMN_WIDTH);
	float posInCol = y * COLUMN_WIDTH + floor(mod(x, COLUMN_WIDTH));
	float xNew = floor(mod(posInCol, IN_TEX_SIZE.x) / 2.0f) * 2.0f;
	float yNew = 68.0f + col * 102.0f + floor(posInCol / IN_TEX_SIZE.x);
	vec2 uv = vec2((xNew / IN_TEX_SIZE.x) + D.x, (yNew / IN_TEX_SIZE.y) + D.y);
	return uv;
}

void main() {
	vec2 coord = vec2(texCoord.x, texCoord.y);	

	// Using plain texture coordinates for performance testing
	//vec2 coordY = coord;
	//vec2 coordU = coord;

	// Calculate NV12 SAND128 on the fly
	vec2 coordY = getYCoord(coord);
	vec2 coordU = getUVCoord(coord);
	vec2 coordV = coordU + vec2(STEP.x, 0.0f);

	float y = texture(yuvTexture, coordY).r;
	float u = texture(yuvTexture, coordU).r - 0.5f;
	float v = texture(yuvTexture, coordV).r - 0.5f;
	
	float r = y + (1.403f * v);
	float g = y - (0.344f * u) - (0.714f * v);
	float b = y + (1.770f * u);

	vec4 col = vec4(r, g, b, 1.0f);

	// Mix images
	fragColor = col;
}
