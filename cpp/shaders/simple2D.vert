/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/LICENSE
 * for full license details.
 */

#version 300 es

layout(location = 0) in vec2 in_Position;
layout(location = 1) in vec2 in_TexCoord;

out vec4 color;
out vec2 texCoord;

const lowp vec4 white = vec4(1.0);

void main() {
	color = white;
	texCoord = in_TexCoord; 
	gl_Position = vec4(in_Position, 0.0, 1.0);
}
