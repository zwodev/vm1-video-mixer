#include <string>
#include <map>
#include <variant>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

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
        this->defaultValue = value;
        this->min = min;
        this->max = max;
        this->step = step;
    }

    void reset() {
        value = defaultValue;
    } 

    std::string name;  
    int value = 0;
    int defaultValue = 0; 
    int min = 0;
    int max = 100;
    int step = 1;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(name),
            CEREAL_NVP(value),
            CEREAL_NVP(defaultValue),
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
        this->defaultValue = value;
        this->min = min;
        this->max = max;
        this->step = step;
    }

    void reset() {
        value = defaultValue;
    } 

    std::string name; 
    float value = 0.0f;
    float defaultValue = 0.0f;
    float min = 0.0f;
    float max = 1.0f;
    float step = 0.01f;


    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(name),
            CEREAL_NVP(value),
            CEREAL_NVP(defaultValue),
            CEREAL_NVP(min),
            CEREAL_NVP(max),
            CEREAL_NVP(step)
        );
    }  
};

struct Vec2Parameter
{
    Vec2Parameter() = default;
    Vec2Parameter(const std::string& name, glm::vec2 value, glm::vec2 min = glm::vec2(0.0f, 0.0f), glm::vec2 max = glm::vec2(1.0f, 1.0f), glm::vec2 step = glm::vec2(0.01f, 0.01f)) {
        this->name = name;
        this->value = value;
        this->defaultValue = value;
        this->min = min;
        this->max = max;
        this->step = step;
    }

    void reset() {
        value = defaultValue;
    } 
    
    std::string name;
    glm::vec2 value;
    glm::vec2 defaultValue; 
    glm::vec2 min = glm::vec2(0.0f, 0.0f);
    glm::vec2 max = glm::vec2(1.0f, 1.0f);
    glm::vec2 step = glm::vec2(0.01f, 0.01f);

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(name),
            CEREAL_NVP(value),
            CEREAL_NVP(defaultValue),
            CEREAL_NVP(min),
            CEREAL_NVP(max),
            CEREAL_NVP(step)
        );
    }  
};

// struct Vec3Parameter
// {
//     Vec3Parameter() = default;
//     Vec3Parameter(const std::string& name, glm::vec3 value) {
//         this->name = name;
//         this->value = value;
//         this->defaultValue = value;
//     }
    
//     std::string name;
//     glm::vec3 value;
//     glm::vec3 defaultValue;
//     glm::vec3 min = glm::vec3(0.0f, 0.0f, 0.0f);
//     glm::vec3 max = glm::vec3(1.0f, 1.0f, 1.0f);
//     glm::vec3 step = glm::vec3(0.01f, 0.01f, 0.01f);


//     template <class Archive>
//     void serialize(Archive& ar)
//     {
//         ar(
//             CEREAL_NVP(name),
//             CEREAL_NVP(value),
//             CEREAL_NVP(defaultValue),
//             CEREAL_NVP(min),
//             CEREAL_NVP(max),
//             CEREAL_NVP(step)
//         );
//     }    
// };