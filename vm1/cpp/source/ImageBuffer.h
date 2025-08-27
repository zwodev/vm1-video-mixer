#pragma once

struct ImageBuffer
{
    ImageBuffer() {}

    ImageBuffer(int width, int height, int channels, char* data) {
        this->width = width;
        this->height = height;
        this->channels = channels;
        this->data = data;
        this->isValid = true;
    }

    ~ImageBuffer() {
        if (data) {
            delete data;
        }
    }

    bool isValid = false;
    int width = 0;
    int height = 0;
    int channels = 0;
    char* data = nullptr;
};