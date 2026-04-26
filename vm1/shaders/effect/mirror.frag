/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

uniform int enabled;        // { "name": "Enabled", "group": "Mirror", "default": 1, "min": 0, "max": 1 }
uniform int axis;           // { "name": "Horizontal/Vertical", "group": "Mirror", "default": 0, "min": 0, "max": 1, "step": 1 }
uniform float axisPos;      // { "name": "Axis Position", "group": "Mirror", "default": 0.5, "min": 0.0, "max": 1.0, "step": 0.01 }


void extMain(inout vec4 color, in vec2 coord)
{
    vec2 uv = coord;
    if(axis == 0)
    {
        float s = min(floor(1.0 + axisPos - coord.y), 1.0);
        
        if(axisPos < 0.5)
            uv.y = mix(coord.y, axisPos*2.0 - coord.y, s);
        else
            uv.y = mix(axisPos*2.0 - coord.y, coord.y, s);
    }
    else 
    {
        float s = min(floor(1.0 + axisPos - coord.x), 1.0);
        
        if(axisPos < 0.5)
            uv.x = mix(coord.x, axisPos*2.0 - coord.x, s);
        else
            uv.x = mix(axisPos*2.0 - coord.x, coord.x, s);
    }

    color = enabled == 1 ? colorAtUV(uv) : color;
}