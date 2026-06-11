/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include "Parameters.h"

#include <string>
#include <map>
#include <variant>
#include <vector>

#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/variant.hpp>





struct ShaderConfig
{
    ShaderConfig() = default;
    // ShaderConfig(const std::string& name) {
    //     this->name = name;
    // }

    void reset() {
        params.clear();
        groups.clear();
    }

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

    std::map<std::string, std::variant<IntParameter, FloatParameter, Vec2Parameter>> params;
    std::map<std::string, std::vector<std::string>> groups;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(params)
        );
    }
};