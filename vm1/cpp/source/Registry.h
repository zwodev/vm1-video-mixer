#pragma once

#include <map>
#include <string>

class Registry {
public:
    Registry() {}
    ~Registry() {}

    void addEntry(int id, const std::string& value) {
        m_entries[id] = value;
        printf("Set -> ID: %d, Value: %s\n", id, value.c_str());
    }

    std::string getValue(int id) {
        std::string value;

        if (m_entries.find(id) != m_entries.end()) {
            value = m_entries[id];
            printf("Get -> ID: %d, Value: %s\n", id, value.c_str());
        }

        return value;
    }

    std::map<int, std::string> m_entries;
};