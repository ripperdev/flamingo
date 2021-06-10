#pragma once

#include <string>
#include <list>
#include <mutex>

#include "base/Singleton.h"

class FileManager final : public Singleton<FileManager> {
public:
    FileManager() = default;

    FileManager(const FileManager &rhs) = delete;

    FileManager &operator=(const FileManager &rhs) = delete;

    bool init(const char *basepath);

    bool isFileExsit(const char *filename);

    void addFile(const char *filename);

private:
    //上传的文件都以文件的md5值命名
    std::list<std::string> m_listFiles;
    std::mutex m_mtFile;
    std::string m_basepath;
};
