/** 
 *  简单的配置文件读取类，ConfigFileReader.h
 *  zhangyl 2017.05.27
 */
#ifndef __CONFIG_FILE_READER_H__
#define __CONFIG_FILE_READER_H__

#include <map>
#include <string>

class CConfigFileReader {
public:
    explicit CConfigFileReader(const char *filename);

    char *getConfigName(const char *name);

    int setConfigValue(const char *name, const char *value);

private:
    void loadFile(const char *filename);

    int writeFile();

    void parseLine(char *line);

    static char *trimSpace(char *name);

    bool m_load_ok = false;
    std::map<std::string, std::string> m_config_map;
    std::string m_config_file;
};


#endif //!__CONFIG_FILE_READER_H__
