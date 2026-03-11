#pragma once

#include <string>
#include <map>
#include <variant>
#include <vector>

#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/variant.hpp>

struct IntParameter
{
    IntParameter() = default;
    IntParameter(const std::string& name, int value, int min = 0, int max = 100, int step = 1) {
        this->name = name;
        this->value = value;
        this->min = min;
        this->max = max;
        this->step = step;
    }

    std::string name;  
    int value = 0; 
    int min = 0;
    int max = 100;
    int step = 1;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(name),
            CEREAL_NVP(value),
            CEREAL_NVP(min),
            CEREAL_NVP(max),
            CEREAL_NVP(step)
        );
    }  
};

struct FloatParameter
{
    FloatParameter() = default;
    FloatParameter(const std::string& name, float value, float min = 0.0f, float max = 1.0f, float step = 0.01f) {
        this->name = name;
        this->value = value;
        this->min = min;
        this->max = max;
        this->step = step;
    }

    std::string name; 
    float value = 0.0f;
    float min = 0.0f;
    float max = 1.0f;
    float step = 0.01f;


    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(name),
            CEREAL_NVP(value),
            CEREAL_NVP(min),
            CEREAL_NVP(max),
            CEREAL_NVP(step)
        );
    }  
};

struct Vec2Parameter
{
    Vec2Parameter() = default;
    Vec2Parameter(const std::string& name, float x, float y) {
        this->name = name;
        this->x = x;
        this->y = y;
    }
    
    std::string name;
    float x = 0.0f;
    float y = 0.0f;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(name),
            CEREAL_NVP(x),
            CEREAL_NVP(y)
        );
    }  
};

struct Vec3Parameter
{
    Vec3Parameter() = default;
    Vec3Parameter(const std::string& name, float x, float y, float z) {
        this->name = name;
        this->x = x;
        this->y = y;
        this->z = z;
    }
    
    std::string name;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(name),
            CEREAL_NVP(x),
            CEREAL_NVP(y),
            CEREAL_NVP(z)
        );
    }    
};

// struct ColorParameter
// {
//     ColorParameter() = default;
//     ColorParameter(const std::string& name, float r, float g, float b, float a = 1.0f) {
//         this->name = name;
//         this->r = r;
//         this->g = g;
//         this->b = b;
//         this->a = a;
//     }
    
//     std::string name;
//     float r = 0.0f;
//     float g = 0.0f;
//     float b = 0.0f;
//     float a = 1.0f;
// };

struct ShaderConfig
{
    ShaderConfig() = default;
    ShaderConfig(const std::string& name) {
        this->name = name;
    }

    std::string name;
    std::map<std::string, std::variant<IntParameter, FloatParameter, Vec2Parameter, Vec3Parameter>> params;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(name),
            CEREAL_NVP(params)
        );
    }
};