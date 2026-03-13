/*
 * Copyright (c) 2023-2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

/*
 * http://www.pouet.net/prod.php?which=57245
 * If you intend to reuse this shader, please add credits to 'Danilo Guanabara'
 * https://www.shadertoy.com/view/XsXXDn
 */

#version 310 es

precision mediump float;

in vec2 texCoord;
out vec4 fragColor;

const vec2 OUT_TEX_SIZE = vec2(1920.0f, 1080.0f);
uniform float iTime;


void main() {
	vec2 fragCoord = gl_FragCoord.xy;
	vec2 r = OUT_TEX_SIZE;

	vec3 c;
	float l,z = iTime;
	for(int i=0; i<3; i++) {
		vec2 uv, p = fragCoord.xy / r;
		uv = p;
		p-=.5;
		p.x *= r.x / r.y;
		z+=.07;
		l=length(p);
		uv+=p/l*(sin(z)+1.)*abs(sin(l*9.-z-z));
		c[i]=.01/length(mod(uv,1.)-.5);
	}
	fragColor = vec4(c/l, iTime);
}


