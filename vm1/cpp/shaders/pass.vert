/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#version 310 es

layout(location = 0) in vec2 in_Position;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in float in_Offset;

out vec4 color;
out vec2 texCoord;
out float offset;

const lowp vec4 white = vec4(1.0);

void main() {
	color = white;
	//texCoord = vec2(in_TexCoord.y, in_TexCoord.x); 
	texCoord = in_TexCoord; 
	offset = in_Offset;
	gl_Position = vec4(in_Position, 0.0, 1.0);
}
