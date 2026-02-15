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

const vec2 OUT_TEX_SIZE = vec2(1920.0f, 1080.0f);
uniform float iTime;

void main() {
	vec2 fragCoord = gl_FragCoord.xy;	// for shaders from shadertoy
	vec2 iResolution = OUT_TEX_SIZE;	// for shaders from shadertoy

	vec2 coord = vec2(texCoord.x, texCoord.y);	
	fragColor = vec4((cos(iTime) + 1.0f) * 0.5f, 0.0f, (sin(iTime) + 1.0f) * 0.5f, 1.0f);
}
