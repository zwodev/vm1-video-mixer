/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#version 300 es

precision mediump float;

in vec4 color;
in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D yuvTexture0;
uniform sampler2D yuvTexture1;
//uniform sampler2D coordTexture;
uniform float mixValue;

const vec2 IN_TEX_SIZE = vec2(2048.0f, 1530.0f);
const vec2 IN_TEX_SIZE_INV = 1.0f / IN_TEX_SIZE;
const vec2 OUT_TEX_SIZE = vec2(1920.0f, 1080.0f);
const vec2 STEP = 1.0f / IN_TEX_SIZE;
const vec2 D = 0.5f / IN_TEX_SIZE;
const float COLUMN_WIDTH = 128.0f;
const float COLUMN_WIDTH_INV = 1.0f / 128.0f;


float columnMod(float x) {
    return x - COLUMN_WIDTH * floor(x * COLUMN_WIDTH_INV);
}

float widthMod(float x) {
    return x - IN_TEX_SIZE.x * floor(x * IN_TEX_SIZE_INV.x);
}

vec2 getYCoord(vec2 coord) {
    // Scale input coordinates to texture space
    float x = floor(coord.x * OUT_TEX_SIZE.x);
    float y = floor(coord.y * OUT_TEX_SIZE.y);

    // Compute column and position within the column
    float col = floor(x * COLUMN_WIDTH_INV);
    float posInCol = y * COLUMN_WIDTH + columnMod(x);

    // Compute new x and y coordinates in the input texture
    float xNew = widthMod(posInCol);
    float yNew = col * 102.0 + floor(posInCol * IN_TEX_SIZE_INV.x);

    // Normalize to UV space and apply offset
    return vec2(xNew * IN_TEX_SIZE_INV.x + D.x, yNew * IN_TEX_SIZE_INV.y + D.y);
}

vec2 getUVCoord(vec2 coord)
{
	float x = floor(coord.x * OUT_TEX_SIZE.x);
	float y = floor(coord.y * OUT_TEX_SIZE.y * 0.5f);
	float col = floor(x * COLUMN_WIDTH_INV);
	float posInCol = y * COLUMN_WIDTH + columnMod(x);
	float xNew = floor(widthMod(posInCol) / 2.0f) * 2.0f;
	float yNew = 68.0f + col * 102.0f + floor(posInCol * IN_TEX_SIZE_INV.x);
	vec2 uv = vec2((xNew * IN_TEX_SIZE_INV.x) + D.x, (yNew * IN_TEX_SIZE_INV.y) + D.y);
	return uv;
}

vec2 getYCoord_OLD(vec2 coord)
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

vec2 getUVCoord_OLD(vec2 coord)
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

	// Using plain texture coordinates for performance testing
	//vec2 coordY = texCoord;
	//vec2 coordU = texCoord;

	// Using pre-calculated NV12 SAND128 coordinates from texture (seems to be slower)
	//vec4 tex = texture(coordTexture, texCoord);
	//vec2 coordY = tex.rg;
	//vec2 coordU = tex.ba;

	// Calculate NV12 SAND128 on the fly
	vec2 coordY = getYCoord(texCoord);
	vec2 coordU = getUVCoord(texCoord);


	vec2 coordV = coordU + vec2(STEP.x, 0.0f);

	float y0 = texture(yuvTexture0, coordY).r;
	float u0 = texture(yuvTexture0, coordU).r - 0.5f;
	float v0 = texture(yuvTexture0, coordV).r - 0.5f;
	//float u0 = y0;
	//float v0 = y0;
	
	float r0 = y0 + (1.403f * v0);
	float g0 = y0 - (0.344f * u0) - (0.714f * v0);
	float b0 = y0 + (1.770f * u0);

	vec4 col0 = vec4(r0, g0, b0, 1.0f);

	// Second image
	float y1 = texture(yuvTexture1, coordY).r;
	float u1 = texture(yuvTexture1, coordU).r - 0.5f;
	float v1 = texture(yuvTexture1, coordV).r - 0.5f;
	//float u1 = y1;
	//float v1 = y1;
	
	float r1 = y1 + (1.403f * v1);
	float g1 = y1 - (0.344f * u1) - (0.714f * v1);
	float b1 = y1 + (1.770f * u1);

	vec4 col1 = vec4(r1, g1, b1, 1.0f);

	// Mix images
	fragColor = mix(col0, col1, mixValue);
	//fragColor = 0.1f * col0 + 0.9f * col1;
	//fragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	//vec4 test = texture(coordTexture, texCoord);
	//fragColor = test;
	//fragColor = vec4(test.a, test.a, test.a, 1.0f);
	//fragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}
