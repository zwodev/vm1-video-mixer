/*
 * Copyright (c) 2023-2025 Nils Zweiling
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
uniform sampler2D yuyvTexture0;
uniform float mixValue;

const vec2 IN_TEX_SIZE = vec2(2048.0f, 1530.0f);
const vec2 IN_TEX_SIZE_INV = 1.0f / IN_TEX_SIZE;
const vec2 OUT_TEX_SIZE = vec2(1920.0f, 1080.0f);
const vec2 STEP = 1.0f / IN_TEX_SIZE;
const vec2 D = 0.5f / IN_TEX_SIZE;
const float COLUMN_WIDTH = 128.0f;
const float COLUMN_WIDTH_INV = 1.0f / 128.0f;

vec2 getYCoordFromNV12(vec2 coord)
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

vec2 getUVCoordFromNV12(vec2 coord)
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

	// Step 1: Calculate NV12 SAND128 (for HEVC video from HW decoder)
	vec2 coordY = getYCoordFromNV12(coord);
	vec2 coordU = getUVCoordFromNV12(coord);
	vec2 coordV = coordU + vec2(STEP.x, 0.0f);

	float y_1 = texture(yuvTexture, coordY).r;
	float u_1 = texture(yuvTexture, coordU).r - 0.5f;
	float v_1 = texture(yuvTexture, coordV).r - 0.5f;

	// Step 2: YUYV (for camera) 
	float pixelX = floor(coord.x * OUT_TEX_SIZE.x);

    // Sample YUYV texel (contains Y0,U,Y1,V)
    vec4 yuyv = texture(yuyvTexture0, coord);

	float y_2 = (mod(pixelX, 2.0) < 1.0) ? yuyv.g : yuyv.a;
	//float y_2 = yuyv.b;
    float u_2 = yuyv.b - 0.5f;
    float v_2 = yuyv.r - 0.5f;
	//float u_2 = 0.0f;
	//float v_2 = 0.0f;

	float y = mix(y_1, y_2, mixValue);
	float u = mix(u_1, u_2, mixValue);
	float v = mix(v_1, v_2, mixValue);
	
	float r = y + (1.403f * v);
	float g = y - (0.344f * u) - (0.714f * v);
	float b = y + (1.770f * u);

	vec4 col = vec4(r, g, b, 1.0f);

	// Mix images
	fragColor = col;
}
