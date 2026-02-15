/*
 * Copyright (c) 2023-2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

 // https://www.shadertoy.com/view/WXtfDX

#version 310 es

precision mediump float;

in vec2 texCoord;
out vec4 fragColor;

const vec2 OUT_TEX_SIZE = vec2(1920.0f, 1080.0f);
uniform float iTime;



void main() {
	vec2 fragCoord = gl_FragCoord.xy;
	vec2 R = OUT_TEX_SIZE;

    // Normalize & Fix aspect-ratio + Diagonal scroll
    vec2 p = fragCoord/R.y + iTime*.1;
    // Cosine palette
    vec3 c = cos(6.2831853*((p.x+p.y)*.12 - vec3(.0,.33,.66)))*.5 + .5;
    // Scaling
    p *= 10.;
    // Truchet tile ("Smith" variation)
    float h = fract(sin(dot(floor(p), vec2(12.9898, 78.233)))*43758.5453123);
    p = fract(vec2(h < .5 ? p.x : -p.x, p.y));
    float t = 1. - smoothstep(.1, .1+15./R.y, abs(min(length(p), length(1.-p)) - .5));

    fragColor = vec4(t*c, 1.);
}


