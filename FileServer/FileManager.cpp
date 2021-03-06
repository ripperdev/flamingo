#include "FileManager.h"

#include <cstring>

#include "base/Logger.h"
#include "base/Platform.h"

bool FileManager::init(const char *basepath) {
    m_basepath = basepath;

    DIR *dp = opendir(basepath);
    if (dp == nullptr) {
        LOG_ERROR("open base dir error, errno: {}, {}", errno, strerror(errno));

        if (mkdir(basepath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
            return true;

        LOG_ERROR("create base dir error, {} , errno: {}, {}", basepath, errno, strerror(errno));

        return false;
    }

    struct dirent *dirp;
    //struct stat filestat;
    while ((dirp = readdir(dp)) != nullptr) {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
            continue;

        //if (stat(dirp->d_name, &filestat) != 0)
        //{
        //    LOGW << "stat filename: [" << dirp->d_name << "] error, errno: " << errno << ", " << strerror(errno);
        //    continue;
        //}

        m_listFiles.emplace_back(dirp->d_name);
        LOG_INFO("filename: {}", dirp->d_name);
    }

    closedir(dp);

    return true;
}

bool FileManager::isFileExsit(const char *filename) {
    std::lock_guard<std::mutex> guard(m_mtFile);
    //先查看缓存
    for (const auto &iter : m_listFiles) {
        if (iter == filename)
            return true;
    }

    //再查看文件系统
    std::string filepath = m_basepath;
    filepath += filename;
    FILE *fp = fopen(filepath.c_str(), "r");
    if (fp != nullptr) {
        fclose(fp);
        m_listFiles.emplace_back(filename);
        return true;
    }

    return false;
}

void FileManager::addFile(const char *filename) {
    std::lock_guard<std::mutex> guard(m_mtFile);
    m_listFiles.emplace_back(filename);
}
