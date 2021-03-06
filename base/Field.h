#pragma once

#include <algorithm>
#include <string>

inline void toLowerString(std::string &str) {
    for (char &i : str) {
        if (i >= 'A' && i <= 'Z') {
            i = i + ('a' - 'A');
        }
    }
}

class Field {
public:

    enum DataTypes {
        DB_TYPE_UNKNOWN = 0x00,
        DB_TYPE_STRING = 0x01,
        DB_TYPE_INTEGER = 0x02,
        DB_TYPE_FLOAT = 0x03,
        DB_TYPE_BOOL = 0x04
    };

    Field();

    Field(Field &f);

    Field(const char *value, enum DataTypes type);

    ~Field() = default;

    enum DataTypes getType() const { return m_iType; }

    std::string getString() const { return m_strValue; }

    std::string getCppString() const {
        return m_strValue;                    // std::string s = 0 have undefine result in C++
    }

    float getFloat() const { return static_cast<float>(atof(m_strValue.c_str())); }

    bool getBool() const { return atoi(m_strValue.c_str()) > 0; }

    int32_t getInt32() const { return static_cast<int32_t>(atol(m_strValue.c_str())); }

    uint8_t getUInt8() const { return static_cast<uint8_t>(atol(m_strValue.c_str())); }

    uint16_t getUInt16() const { return static_cast<uint16_t>(atol(m_strValue.c_str())); }

    int16_t getInt16() const { return static_cast<int16_t>(atol(m_strValue.c_str())); }

    uint32_t getUInt32() const { return static_cast<uint32_t>(atol(m_strValue.c_str())); }

    uint64_t getUInt64() const {
        uint64_t value = 0;
        value = atoll(m_strValue.c_str());
        return value;
    }

    void setType(enum DataTypes type) { m_iType = type; }

    void setValue(const char *value, size_t uLen);

    void setName(const std::string &strName) {
        m_strFieldName = strName;
        toLowerString(m_strFieldName);
    }

    const std::string &getName() {
        return m_strFieldName;
    }

    bool isNULL() const {
        return m_bNULL;
    }

public:
    bool m_bNULL{};

private:
    std::string m_strValue;
    std::string m_strFieldName;
    enum DataTypes m_iType;
};
