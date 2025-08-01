/*
 * Copyright (c) 2023-2025 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 *
 * Parts of this file have been taken from:
 * https://github.com/libsdl-org/SDL/blob/main/test/testffmpeg.c
 *
 */

#include "VM1Application.h"

int main(int, char **)
{
    VM1Application vm1;
    if (!vm1.exec()) return 1;
    // while (true) {
    //     if(!vm1.exec()) break;
    // };

    return 0;
}
