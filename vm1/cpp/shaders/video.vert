/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */
 
#version 310 es

layout(location = 0) in vec2 in_Position;
layout(location = 1) in vec2 in_TexCoord;

uniform int stripId;           // 0..15
uniform float stripWidthNDC;   // e.g. 0.125 for 16 stripes

out vec2 texCoord;

void main() {
    // Compute horizontal offset for this strip in NDC
    float xOffset = float(stripId) * stripWidthNDC - 1.0;

    // Scale quad width to match strip width, and offset by strip id
    vec2 pos = in_Position;
    pos.x = pos.x * (stripWidthNDC * 0.5) + xOffset + stripWidthNDC * 0.5;

    gl_Position = vec4(pos, 0.0, 1.0);
    texCoord = in_TexCoord;
}