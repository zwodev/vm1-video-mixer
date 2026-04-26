/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

uniform int enabled;        // { "name": "Enabled", "group": "Fragments", "default": 1, "min": 0, "max": 1 }
uniform int tilesX;         // { "name": "Tiles X", "group": "Fragments", "default": 4, "min": 1, "max": 10, "step": 1 }
uniform int tilesY;         // { "name": "Tiles Y", "group": "Fragments", "default": 4, "min": 1, "max": 10, "step": 1 }
uniform float dispAmount;   // { "name": "Amount" , "group": "Fragments", "default": 0.1, "min": 0.0, "max": 1.0, "step": 0.1 }  
uniform int seed;           // { "name": "Seed", "group": "Fragments", "default": 24234, "min": 0, "max": 100000, "step": 1 }

float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}



void extMain(inout vec4 color, in vec2 coord)
{
    if(enabled == 0) return;

    vec2 uv = coord;
    vec2 tilesCount = vec2(float(tilesX),float(tilesY));

    vec2 tileSize = 1.0 / tilesCount;
    // tileSize is e.g. 0.25 (with 4 tiles)

    vec2 tileIndex = floor(uv / tileSize);
    // tileIndex is 0, 1, 2, 3 (with 4 tiles)

    float rnd = hash(tileIndex);

    vec2 normalizedTileIndex = rnd / tilesCount;
    // normalizedTileIndex is 0.0, 0.25, 0.5, 0.75  (with 4 tiles)

    vec2 tileUV = fract(uv / tileSize);

    float r1 = hash(tileIndex);
    float r2 = hash(tileIndex + 17.23 + float(seed));

    vec2 offset = (vec2(r1, r2) - 0.5) * dispAmount;
    vec2 sampleUV = uv + offset;

    vec2 randomTile = floor(vec2(
        hash(tileIndex),
        hash(tileIndex + 9.17)
    ) * tilesCount);

    vec2 srcUV = (randomTile + tileUV) / tilesCount;
    color = colorAtUV(srcUV);
}

