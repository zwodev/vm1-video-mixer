/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

uniform int enabled;        // { "name": "Enabled", "group": "Fragments", "default": 1, "min": 0, "max": 1 }
uniform vec2 tiles;         // { "name": "Tiles", "group": "Fragments", "default": [4,4], "min": [1,1], "max": [100,100], "step": [1,1] }
uniform int seed;           // { "name": "Seed", "group": "Fragments", "default": 24234, "min": 0, "max": 100000, "step": 1 }



// Feistel-Rundfunktion
float fRound(float x, float key) {
    return fract(sin(x * 127.1 + key) * 43758.5453);
}

// Bijektive Permutation für Index x in [0, N)
// Feistel-Netzwerk: N wird auf m×m aufgespannt, 4 Runden
int feistelPermute(int x, int N, int sk) {
    int m  = int(ceil(sqrt(float(N))));   // Blockhälfte
    float fm = float(m);
    float fsk = float(sk) * 0.00017;

    int l = x / m;
    int r = x % m;

    // 4 Feistel-Runden
    for (int i = 0; i < 4; i++) {
        int tmp = r;
        r = int(mod(
                float(l) + fRound(float(r) / fm, fsk + float(i) * 2.718),
                fm)) ;
        // Wichtig: ganzzahlig runden
        r = int(mod(float(l) + floor(fRound(float(tmp) / fm, fsk + float(i) * 2.718) * fm), fm));
        l = tmp;
    }

    // Zurück auf [0, N) falten (cycle-walking-Approximation)
    return (l * m + r) % N;
}

void extMain(inout vec4 color, in vec2 coord) {
    vec2 uv = coord;
    vec2 tileSize = 1.0 / tiles;

    vec2 tileIdx = floor(uv / tileSize);
    vec2 localUV = fract(uv / tileSize);

    // Linearer Index der aktuellen Kachel
    int linearIdx = int(tileIdx.y) * int(tiles.x) + int(tileIdx.x);
    int N         = int(tiles.x) * int(tiles.y);

    // Permutierter Quell-Index
    int srcLinear = feistelPermute(linearIdx, N, seed);

    // Zurück in 2D
    vec2 srcTile = vec2(
        float(srcLinear % int(tiles.x)),
        float(srcLinear / int(tiles.x))
    );

    vec2 srcUV = (srcTile + localUV) / tiles;

    color = colorAtUV(srcUV);
}