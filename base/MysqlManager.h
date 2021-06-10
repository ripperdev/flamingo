#pragma once

#include <memory>
#include <map>
#include <utility>
#include <vector>

#include "DatabaseMysql.h"

#define MAXCMDLEN 8192

struct STableField {
    STableField() = default;

    STableField(std::string strName, std::string strType, std::string strIndex) :
            m_strName(std::move(strName)),
            m_strType(std::move(strType)),
            m_strDesc(std::move(strIndex)) {
    }

    std::string m_strName;
    std::string m_strType;
    std::string m_strDesc;
};

struct STableInfo {
    STableInfo() = default;

    explicit STableInfo(std::string strName)
            : m_strName(std::move(strName)) {
    }

    std::string m_strName;
    std::map<std::string, STableField> m_mapField;
    std::string m_strKeyString;
};

class CMysqlManager {
public:
    CMysqlManager();

    virtual ~CMysqlManager() = default;

public:
    bool init(const char *host, const char *user, const char *pwd, const char *dbname);

    std::string getHost() { return m_strHost; }

    std::string getUser() { return m_strUser; }

    std::string getPwd() { return m_strPassword; }

    std::string getDBName() { return m_strDataBase; }

    std::string getCharSet() { return m_strCharactSet; }

private:
    bool isDBExist();

    bool createDB();

    bool checkTable(const STableInfo &table);

    bool createTable(const STableInfo &table);

protected:
    std::shared_ptr<CDatabaseMysql> m_poConn;

    std::string m_strHost;
    std::string m_strUser;
    std::string m_strPassword;
    std::string m_strDataBase;
    std::string m_strCharactSet;

    std::vector<STableInfo> m_vecTableInfo;
};
