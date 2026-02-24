#pragma once

#include <string>
#include <map>
#include <variant>
#include <vector>

struct IntParameter
{
    IntParameter(const std::string& name, int value, int min = 0, int max = 100, int step = 1) {
        this->name = name;
        this->value = value;
        this->min = min;
        this->max = max;
        this->step = step;
    }

    std::string name;   
    int min = 0;
    int max = 100;
    int step = 1;
    int value = 0;
};

struct FloatParameter
{
    FloatParameter(const std::string& name, float value, float min = 0.0f, float max = 1.0f, float step = 0.01f) {
        this->name = name;
        this->value = value;
        this->min = min;
        this->max = max;
        this->step = step;
    }

    std::string name; 
    float min = 0.0f;
    float max = 1.0f;
    float step = 0.01f;
    float value = 0.0f;
};

struct ColorParameter
{
    ColorParameter(const std::string& name, float r, float g, float b, float a = 1.0f) {
        this->name = name;
        this->r = r;
        this->g = g;
        this->b = b;
        this->a = a;
    }
    
    std::string name;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

struct ShaderConfig
{
    ShaderConfig(const std::string& name) {
        this->name = name;
    }

    std::string name;
    std::vector<std::variant<IntParameter, FloatParameter, ColorParameter>> params;
};