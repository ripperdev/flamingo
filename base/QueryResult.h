#pragma once

#include <mysql/mysql.h>
#include <cstdint>
#include <vector>
#include <map>

#include "Field.h"

class QueryResult {
public:
    typedef std::map<uint32_t, std::string> FieldNames;

    QueryResult(MYSQL_RES *result, uint64_t rowCount, uint32_t fieldCount);

    virtual ~QueryResult();

    virtual bool nextRow();

    uint32_t getField_idx(const std::string &name) const {
        for (const auto &iter : getFieldNames()) {
            if (iter.second == name)
                return iter.first;
        }

        //assert(false && "unknown field name");
        return uint32_t(-1);
    }

    Field *fetch() const { return m_CurrentRow; }

    const Field &operator[](int index) const {
        return m_CurrentRow[index];
    }

    const Field &operator[](const std::string &name) const {
        return m_CurrentRow[getField_idx(name)];
    }

    uint32_t getFieldCount() const { return m_FieldCount; }

    uint64_t getRowCount() const { return m_RowCount; }

    FieldNames const &getFieldNames() const { return m_FieldNames; }

    std::vector<std::string> const &getNames() const { return m_vtFieldNames; }

private:
    static enum Field::DataTypes convertNativeType(enum_field_types mysqlType) ;

public:
    void endQuery();

protected:
    Field *m_CurrentRow;
    uint32_t m_FieldCount;
    uint64_t m_RowCount;
    FieldNames m_FieldNames;
    std::vector<std::string> m_vtFieldNames;

    MYSQL_RES *m_Result;
};
