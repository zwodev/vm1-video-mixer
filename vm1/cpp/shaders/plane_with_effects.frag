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

// standard
uniform sampler2D inputTexture0;
uniform sampler2D inputTexture1;
uniform float mixValue;
uniform float opacity;

// effects
uniform float ColorCorrection_Brightness;
uniform float ColorCorrection_Contrast;
uniform float ColorCorrection_Saturation;

vec3 adjustBrightness(vec3 color, float value) {
  return color + value;
}

vec3 adjustContrast(vec3 color, float value) {
  return 0.5 + (1.0 + value) * (color - 0.5);
}

vec3 adjustSaturation(vec3 color, float value) {
  // https://www.w3.org/TR/WCAG21/#dfn-relative-luminance
  const vec3 luminosityFactor = vec3(0.2126, 0.7152, 0.0722);
  vec3 grayscale = vec3(dot(color, luminosityFactor));

  return mix(grayscale, color, 1.0 + value);
}

// ### BEGIN CUSTOM
// // Internal
// uniform vec2 iResolution;
// uniform flaot iTime;

// // Custom 
// uniform int replications;
// vec3 customMain(vec2 coord, vec3 color) {
//	return color;
// }
// ### END CUSTOM

void main() {
	// Mix images
	vec2 coord = vec2(texCoord.x, (1.0f - texCoord.y));
	vec3 col0 = texture(inputTexture0, coord).rgb;
	vec3 col1 = texture(inputTexture1, coord).rgb;
	vec3 color = mix(col0, col1, mixValue);

	// color = customShader(color);

	color = adjustSaturation(color, ColorCorrection_Saturation);
	color = adjustContrast(color, ColorCorrection_Contrast);
	color = adjustBrightness(color, ColorCorrection_Brightness);

	color = mix(vec3(1.0f), color, opacity);
	//color = color * opacity;

	fragColor = vec4(color, 1.0f);
}
