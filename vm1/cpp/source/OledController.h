/*
 * Copyright (c) 2025 Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

class OledController
{
public:
    OledController();
    ~OledController();

    void initialize();
    void updateImage();

private:
    // Internal members and variables
};
