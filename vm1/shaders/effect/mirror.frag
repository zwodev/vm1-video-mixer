/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

uniform float xAxis;            // { "name": "horizontal axis", "group": "Mirror", "default": 0.5, "min": 0.0, "max": 1.0, "step": 0.01 }

void extMain(inout vec3 color, in vec2 coord)
{
    float s = min(floor(1.0 + xAxis - coord.y), 1.0);
    vec2 uv = coord;
    
    if(xAxis < 0.5)
        uv.y = mix(coord.y, xAxis*2.0 - coord.y, s);
    else
        uv.y = mix(xAxis*2.0 - coord.y, coord.y, s);

    color = colorAtUV(uv);
}