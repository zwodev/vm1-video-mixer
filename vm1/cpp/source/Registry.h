#pragma once

#include <map>
#include <string>

class Registry {
public:
    Registry() {}
    ~Registry() {}

    void addEntry(int id, const std::string& value) {
        m_idsToValue[id] = value;
        printf("Set -> ID: %d, Value: %s\n", id, value.c_str());
    }

    std::string getValue(int id) {
        std::string value;

        if (m_idsToValue.find(id) != m_idsToValue.end()) {
            value = m_idsToValue[id];
            //printf("Get -> ID: %d, Value: %s\n", id, value.c_str());
        }

        return value;
    }

    int getId(const std::string& value) {
        for (const auto& pair : m_idsToValue) {
            if (pair.second == value) {
               return pair.first;
            }
        }

        return -1;
    }

    std::map<int, std::string> m_idsToValue;
};