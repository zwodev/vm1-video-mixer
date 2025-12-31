#pragma once

#include <cstdlib>  // for free()
#include <utility>  // for std::move
#include <string>
#include "stb/stb_image.h"  // For stbi_load function declaration

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

    ImageBuffer(const std::string& filename) {
        int w, h, ch;
        unsigned char* imgData = stbi_load(filename.c_str(), &w, &h, &ch, 0);
        
        if (imgData == nullptr) {
            isValid = false;
            width = 0;
            height = 0;
            channels = 0;
            data = nullptr;
            return;
        }
        
        width = w;
        height = h;
        channels = ch;
        data = reinterpret_cast<char*>(imgData);
        isValid = true;
    }

    // Delete copy constructor and copy assignment to prevent double-free bugs.
    // If ImageBuffer were copyable, two objects could point to the same data pointer.
    // When both destructors run, they would both try to free() the same memory,
    // causing a crash. Instead, we use move semantics (below) to transfer ownership.
    ImageBuffer(const ImageBuffer&) = delete;
    ImageBuffer& operator=(const ImageBuffer&) = delete;

    // Move constructor
    ImageBuffer(ImageBuffer&& other) noexcept 
        : isValid(other.isValid), width(other.width), height(other.height), 
          channels(other.channels), data(other.data) {
        other.data = nullptr;  // Prevent other from freeing
        other.isValid = false;
    }

    // Move assignment
    ImageBuffer& operator=(ImageBuffer&& other) noexcept {
        if (this != &other) {
            // Free our current data
            if (data) {
                free(data);
            }
            // Take ownership of other's data
            isValid = other.isValid;
            width = other.width;
            height = other.height;
            channels = other.channels;
            data = other.data;
            // Prevent other from freeing
            other.data = nullptr;
            other.isValid = false;
        }
        return *this;
    }

    ~ImageBuffer() {
        if (data) {
            // Data comes from stbi_load which uses malloc, so use free()
            free(data);
        }
    }

    bool isValid = false;
    int width = 0;
    int height = 0;
    int channels = 0;
    char* data = nullptr;
};