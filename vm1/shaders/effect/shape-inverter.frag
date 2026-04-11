/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */
 
uniform int enabled;        // { "name": "Enabled", "group": "Shape Inverter", "default": 1, "min": 0, "max": 1 }
uniform int shape;          // { "name": "Shape Type", "group": "Shape Inverter", "default": 0, "min": 0, "max": 2 }
uniform float r;            // { "name": "Radius", "group": "Shape Inverter", "default": 0.2, "min": 0.0, "max": 1.0, "step": 0.01 }
uniform float strokeWidth;  // { "name": "Stroke Width", "group": "Shape Inverter", "default": 0.2, "min": 0.01, "max": 1.0, "step": 0.01 }
//uniform vec2 pos;         // { "name": "Position", "group": "Shape Inverter", "default": [0.0, 0,0], "min": [-1.0, -1,0], "max":[1.0, 1,0], "step": [0.01, 0.01] }
uniform float px;           // { "name": "pos x", "group": "Shape Inverter", "default": 0.0, "min": -1.0, "max": 1.0, "step": 0.01 }
uniform float py;           // { "name": "pos y", "group": "Shape Inverter", "default": 0.0, "min": -1.0, "max": 1.0, "step": 0.01 }
uniform float threshold;    // { "name": "Threshold", "group": "Shape Inverter", "default": 0.0, "min": 0.0, "max": 1.0, "step": 0.01 }

float sdCircle(vec2 p, float r) {
    return length(p) - r;
}

float sdBox(vec2 p, float r) {
    vec2 q = abs(p) - r;
    return length(max(q,0.0)) + min(max(q.x,q.y),0.0);
}

float sdTriangle(vec2 p, float r) {
    const float k = sqrt(3.0);
    p.x = abs(p.x) - r;
    p.y = p.y + r/k;
    if( p.x+k*p.y > 0.0 ) p=vec2(p.x-k*p.y, -k*p.x-p.y) / 2.0;
    p.x = p.x - clamp( p.x, -2.0*r, 0.0 );
    return -length(p) * sign(p.y);
}

float shapeSDF(vec2 p, int shape, float r) {
    float d1 = sdCircle(p, r);
    float d2 = sdBox(p, r * 0.8);
    float d3 = sdTriangle(p, r);
    
    // 0=circle → 1=box → 2=triangle
    float t1 = smoothstep(0.0, 1.0, float(shape));
    float t2 = smoothstep(1.0, 2.0, float(shape));
    
    return mix(mix(d1, d2, t1), d3, t2);
}

float shapeMask(vec2 p, float r, float strokeWidth, int shape) {
    float dist = shapeSDF(p, shape, r);
    float outerMask = smoothstep(0.0, 0.001, -dist);
    float innerMask = smoothstep(0.0, 0.001, dist + strokeWidth);
    return outerMask * innerMask;
}


void extMain(inout vec3 color, in vec2 coord)
{
    vec2 p = coord - 0.5;
    p.y /= iAspect;
    // p += pos;
    p.y += py;
    p.x += px;
    vec2 uvMirrored = coord;
    uvMirrored.y = 1.0-uvMirrored.y;
    vec3 colorMirrored = colorAtUV(uvMirrored);
    
    float brightness = (color.r + color.g + color.b) / 3.0;
    float maskValue = shapeMask(p, r, strokeWidth, shape) * step(threshold, brightness); // maybe use smoothstep?

    // color = mix(color, 1.0 - color, maskValue * float(enabled));
    color = mix(color, colorMirrored, maskValue * float(enabled));
}