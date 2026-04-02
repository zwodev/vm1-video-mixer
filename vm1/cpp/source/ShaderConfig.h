/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

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

struct Vec2
{
    Vec2() = default;
    Vec2(float x, float y) {
        this->x = x;
        this->y = y;
    }

    float& operator[](std::size_t i) {
        if (i == 0) return x;
        else if (i == 1) return y;
        else throw std::out_of_range("Vec2 index");
    }

    const float& operator[](std::size_t i) const {
        if (i == 0) return x;
        else if (i == 1) return y;
        else throw std::out_of_range("Vec2 index");
    }

    float x = 0.0f;
    float y = 0.0f;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(x),
            CEREAL_NVP(y)
        );
    }  
};

struct Vec2Parameter
{
    Vec2Parameter() = default;
    Vec2Parameter(const std::string& name, Vec2 value, Vec2 min = Vec2(0.0f, 0.0f), Vec2 max = Vec2(1.0f, 1.0f), Vec2 step = Vec2(0.01f, 0.01f)) {
        this->name = name;
        this->value = value;
    }
    
    std::string name;
    Vec2 value;
    Vec2 min = Vec2(0.0f, 0.0f);
    Vec2 max = Vec2(1.0f, 1.0f);
    Vec2 step = Vec2(0.01f, 0.01f);

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

struct Vec3
{
    Vec3() = default;
    Vec3(float x, float y, float z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    float& operator[](std::size_t i) {
        if (i == 0) return x;
        else if (i == 1) return y;
        else if (i == 2) return z;
        else throw std::out_of_range("Vec3 index");
    }

    const float& operator[](std::size_t i) const {
        if (i == 0) return x;
        else if (i == 1) return y;
        else if (i == 2) return z;
        else throw std::out_of_range("Vec3 index");
    }

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(x),
            CEREAL_NVP(y),
            CEREAL_NVP(z)
        );
    }  
};

struct Vec3Parameter
{
    Vec3Parameter() = default;
    Vec3Parameter(const std::string& name, Vec3 value) {
        this->name = name;
        this->value = value;
    }
    
    std::string name;
    Vec3 value;
    Vec3 min = Vec3(0.0f, 0.0f, 0.0f);
    Vec3 max = Vec3(1.0f, 1.0f, 1.0f);
    Vec3 step = Vec3(0.01f, 0.01f, 0.01f);


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

struct ShaderConfig
{
    ShaderConfig() = default;
    // ShaderConfig(const std::string& name) {
    //     this->name = name;
    // }

    void update(const ShaderConfig& shaderConfig) {
        std::vector<std::string> toDelete;
        for (const auto& kv : params) {
            const std::string& paramName = kv.first;
            if (!shaderConfig.params.contains(paramName)) {
                toDelete.push_back(paramName);
            }
        }
        for (const auto& paramName : toDelete) {
            params.erase(paramName);
        }
        for (const auto& kv : shaderConfig.params) {
            const std::string& paramName = kv.first;
            if (!params.contains(paramName)) {
                params[paramName] = kv.second;
            }
        }

        groups = shaderConfig.groups;
    }

    std::map<std::string, std::variant<IntParameter, FloatParameter, Vec2Parameter, Vec3Parameter>> params;
    std::map<std::string, std::vector<std::string>> groups;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(params)
        );
    }
};