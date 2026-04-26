/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
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

// constants
const vec2 iResolution = vec2(1920.0f, 1080.0f);
const float iAspect = iResolution.x / iResolution.y;


// standard
uniform sampler2D inputTexture0;
uniform sampler2D inputTexture1;
uniform float mixValue;
uniform float opacity;
uniform int isMultiplication;
uniform float iTime;

// color correction
uniform float ColorCorrection_Brightness; // { "name": "Brightness", "group": "Color Correction", "min": -1.0, "max": 1.0 }
uniform float ColorCorrection_Contrast;	  // { "name": "Contrast",   "group": "Color Correction", "min": -1.0, "max": 1.0 }
uniform float ColorCorrection_Saturation; // { "name": "Saturation", "group": "Color Correction", "min": -1.0, "max": 1.0 }

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

// chroma key (from https://www.shadertoy.com/view/MlVXWD)
uniform int ChromaKey_Enable;      // { "name": "Enable", 	 "group": "Chroma Key", "min": 0,     "max": 1,     "default": 0 }
uniform float ChromaKey_RangeLow;  // { "name": "Range Low", "group": "Chroma Key", "min": 0.001, "max": 0.999, "default": 0.005, "step": 0.001 }
uniform float ChromaKey_RangeHigh; // { "name": "Range Low", "group": "Chroma Key", "min": 0.001, "max": 0.999, "default": 0.26, "step": 0.001 }
uniform float ChromaKey_ColorR;    // { "name": "Red", 		 "group": "Chroma Key", "min": 0.0,   "max": 1.0,   "default": 0.05, "step": 0.01 }
uniform float ChromaKey_ColorG;    // { "name": "Green", 	 "group": "Chroma Key", "min": 0.0,   "max": 1.0,   "default": 0.63, "step": 0.01 }
uniform float ChromaKey_ColorB;    // { "name": "Blue", 	 "group": "Chroma Key", "min": 0.0,   "max": 1.0,   "default": 0.14, "step": 0.01 }

mat4 RGBtoYUV = mat4(0.257,  0.439, -0.148, 0.0,
                     0.504, -0.368, -0.291, 0.0,
                     0.098, -0.071,  0.439, 0.0,
                     0.0625, 0.500,  0.500, 1.0 );

float colorclose(vec3 yuv, vec3 keyYuv, vec2 tol)
{
    float tmp = sqrt(pow(keyYuv.g - yuv.g, 2.0) + pow(keyYuv.b - yuv.b, 2.0));
    if (tmp < tol.x)
      return 0.0;
   	else if (tmp < tol.y)
      return (tmp - tol.x)/(tol.y - tol.x);
   	else
      return 1.0;
}

vec4 colorAtUV(vec2 coord)
{
	vec4 col0 = texture(inputTexture0, coord);
	vec4 col1 = texture(inputTexture1, coord);
	return mix(col0, col1, mixValue);
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

//###EXT_MAIN_DEF###


void main() {
	// Mix images
	vec2 coord = vec2(texCoord.x, (1.0f - texCoord.y));
	vec4 color = colorAtUV(coord);

	//###EXT_MAIN_USE###
	// extMain(color, coord); 

	// chroma key
	float chromaKey_Mask = 0.0;
	vec4 chromaKey = vec4(ChromaKey_ColorR, ChromaKey_ColorG, ChromaKey_ColorB, 1);
	vec2 chromaKey_MaskRange = vec2(ChromaKey_RangeLow, ChromaKey_RangeHigh);
	vec4 keyYUV =  RGBtoYUV * chromaKey;
	vec4 yuv = RGBtoYUV * color;
	chromaKey_Mask = 1.0 - colorclose(yuv.rgb, keyYUV.rgb, chromaKey_MaskRange);
	color = mix(max(color - chromaKey_Mask * chromaKey, 0.0), color, float(1 - ChromaKey_Enable));

	// color correction
	color.rgb = adjustSaturation(color.rgb, ColorCorrection_Saturation);
	color.rgb = adjustContrast(color.rgb, ColorCorrection_Contrast);
	color.rgb = adjustBrightness(color.rgb, ColorCorrection_Brightness);

	color.rgb = mix(color.rgb, mix(vec3(1.0f), color.rgb, opacity * (1.0 - chromaKey_Mask)), float(isMultiplication));

	color.a *= opacity;
	fragColor = color;
}
