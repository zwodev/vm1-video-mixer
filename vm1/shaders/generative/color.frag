/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
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
// uniform vec2 iResolution;
uniform float iTime; // { "name": "Elapsed Time", "default": 0.0, "min": 0.0, "max": 1000000.0, "step": 0.01 }
uniform float red;   // { "name": "Red", "default": 1.0, "min": 0.0, "max":1.0, "step": 0.01 }
uniform float green; // { "name": "Green", "default": 0.5, "min": 0.0, "max":1.0, "step": 0.01 }
uniform float blue;  // { "name": "Blue", "default": 0.5, "min": 0.0, "max":1.0, "step": 0.01 }
uniform float alpha; // { "name": "Transparency", "default": 0.0, "min": 0.0, "max":1.0, "step": 0.01 }

void main() {
	vec2 fragCoord = gl_FragCoord.xy;
	vec2 iResolution = OUT_TEX_SIZE;

	fragColor = vec4(red, green, blue, alpha);
}