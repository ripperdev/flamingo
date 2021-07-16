#ifndef FLAMINGO_LOGGER_H
#define FLAMINGO_LOGGER_H

#include <memory>       // for shared_ptr
#include <string>       // for string

#include "Singleton.h"  // for Singleton

namespace spdlog {
class logger;
}  // namespace spdlog

class Logger : public Singleton<Logger> {
public:
    [[nodiscard]] bool init(const std::string &name, const std::string &file);

    std::shared_ptr<spdlog::logger> &getLogger();

private:
    bool inited = false;
    std::shared_ptr<spdlog::logger> logger = nullptr;
};

#define LOG_TRACE Logger::getMe().getLogger()->trace
#define LOG_DEBUG Logger::getMe().getLogger()->debug
#define LOG_INFO Logger::getMe().getLogger()->info
#define LOG_WARN Logger::getMe().getLogger()->warn
#define LOG_ERROR Logger::getMe().getLogger()->error
#define LOG_CRITICAL Logger::getMe().getLogger()->critical

#endif //FLAMINGO_LOGGER_H
